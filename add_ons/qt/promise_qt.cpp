/*
 * Copyright (c) 2021, xhawk18
 * at gmail.com
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

//
// Promisified timer based on promise-cpp and QT
//
// Functions --
//   Defer yield(QWidget *widget);
//   Defer delay(QWidget *widget, uint64_t time_ms);
//   void cancelDelay(Defer d);
// 
//   Defer setTimeout(QWidget *widget,
//                    const std::function<void(bool /*cancelled*/)> &func,
//                    uint64_t time_ms);
//   void clearTimeout(Defer d);
//

#include "../../include/promise.hpp"
#include "promise_qt.hpp"
#include <chrono>
#include <QObject>
#include <QTimerEvent>
#include <QApplication>

namespace promise {

PromiseEventFilter::PromiseEventFilter() {}

PromiseEventFilter::Listeners::iterator PromiseEventFilter::addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func) {
    std::pair<QObject *, QEvent::Type> key = { object, eventType };
    return listeners_.insert({ key, func });
}

void PromiseEventFilter::removeEventListener(PromiseEventFilter::Listeners::iterator itr) {
    listeners_.erase(itr);
}

PromiseEventFilter &PromiseEventFilter::getSingleInstance() {
    static PromiseEventFilter promiseEventFilter;
    return promiseEventFilter;
}

bool PromiseEventFilter::eventFilter(QObject *object, QEvent *event) {
    std::pair<QObject *, QEvent::Type> key = { object, event->type() };

    //LOG_INFO("eventFilter = {} {}", object, event->type());
    // may not safe if one handler is removed by other
    std::list<Listeners::iterator> itrs;
    for (Listeners::iterator itr = listeners_.lower_bound(key);
        itr != listeners_.end() && key == itr->first; ++itr) {
        itrs.push_back(itr);
    }
    for (Listeners::iterator itr : itrs) {
        itr->second(object, event);
    }

    if (event->type() == QEvent::Destroy) {
        removeObjectFilters(object);
    }

    return QObject::eventFilter(object, event);
}

bool PromiseEventFilter::removeObjectFilters(QObject *object) {
    std::pair<QObject *, QEvent::Type> key = { object, QEvent::None };

    // checked one by one for safety (others may be removed)
    while (true) {
        Listeners::iterator itr = listeners_.lower_bound(key);
        if (itr != listeners_.end() && itr->first.first == object) {
            itr->second(object, nullptr);
        }
        else {
            break;
        }
    }

    return false;
}

// Wait event will wait the event for only once
Defer waitEvent(QObject *object,
                QEvent::Type  eventType,
                bool          callSysHandler) {
    auto listener = std::make_shared<PromiseEventFilter::Listeners::iterator>();
    Defer promise = newPromise([listener, object, eventType, callSysHandler](Callback &cb) {

        std::shared_ptr<bool> disableFilter = std::make_shared<bool>(false);
        //PTI;
        *listener = PromiseEventFilter::getSingleInstance().addEventListener(
            object, eventType, [cb, callSysHandler, disableFilter](QObject *object, QEvent *event) {
            (void)object;
            if (event == nullptr) {
                cb.reject();
                return false;
            }
            // The next then function will be call immediately
            // Be care that do not use event in the next event loop
            else if (*disableFilter) {
                return false;
            }
            else if (callSysHandler) {
                //PTI;
                *disableFilter = true;
                QApplication::sendEvent(object, event);
                *disableFilter = false;
                cb.resolve(event);
                return true;
            }
            else {
                //PTI;
                cb.resolve(event);
                return false;
            }
        }
        );
    }).finally([listener]() {
        //PTI;
        PromiseEventFilter::getSingleInstance().removeEventListener(*listener);
    });

    return promise;
}


QtTimerHolder::QtTimerHolder() {
}

QtTimerHolder::~QtTimerHolder() {
}

Defer QtTimerHolder::delay(int time_ms) {
    int timerId = getInstance().startTimer(time_ms);

    return newPromise([timerId](Callback &cb) {
        getInstance().defers_.insert({ timerId, cb });
    }).finally([timerId]() {
        getInstance().killTimer(timerId);
        getInstance().defers_.erase(timerId);
    });
}

Defer QtTimerHolder::yield() {
    return delay(0);
}

Defer QtTimerHolder::setTimeout(const std::function<void(bool)> &func,
                                int time_ms) {
    return delay(time_ms).then([func]() {
        func(false);
    }, [func]() {
        func(true);
    });
}

void QtTimerHolder::timerEvent(QTimerEvent *event) {
    int timerId = event->timerId();
    auto found = this->defers_.find(timerId);
    if (found != this->defers_.end()) {
        Callback &cb = found->second;
        cb.resolve();
    }
    QObject::timerEvent(event);
}

QtTimerHolder &QtTimerHolder::getInstance() {
    static QtTimerHolder s_qtTimerHolder_;
    return s_qtTimerHolder_;
}

Defer delay(int time_ms) {
    return QtTimerHolder::delay(time_ms);
}

Defer yield() {
    return QtTimerHolder::yield();
}

Defer setTimeout(const std::function<void(bool)> &func,
                 int time_ms) {
    return QtTimerHolder::setTimeout(func, time_ms);
}


void cancelDelay(Defer d) {
    d.reject();
}

void clearTimeout(Defer d) {
    cancelDelay(d);
}

}
