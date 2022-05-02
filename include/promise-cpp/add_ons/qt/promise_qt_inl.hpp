#ifndef INC_PROMISE_QT_CPP_
#define INC_PROMISE_QT_CPP_

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
//   Promise yield(QWidget *widget);
//   Promise delay(QWidget *widget, uint64_t time_ms);
//   void cancelDelay(Promise promise);
// 
//   Promise setTimeout(QWidget *widget,
//                      const std::function<void(bool /*cancelled*/)> &func,
//                      uint64_t time_ms);
//   void clearTimeout(Promise promise);
//

#include "promise-cpp/promise.hpp"
#include "promise_qt.hpp"
#include <chrono>
#include <QObject>
#include <QTimerEvent>
#include <QApplication>

namespace promise {

PromiseEventFilter::PromiseEventFilter() 
    : removeLaters_(ListenersIteratorCompare())
    , eventFilterRecursiveCount_(0) {
}

PromiseEventFilter::Listeners::iterator PromiseEventFilter::addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func) {
    std::pair<QObject *, QEvent::Type> key = { object, eventType };
    return listeners_.insert({ key, func });
}

void PromiseEventFilter::removeEventListener(PromiseEventFilter::Listeners::iterator itr) {
    removeLaters_.insert(itr);
    //listeners_.erase(itr);
}

PromiseEventFilter &PromiseEventFilter::getSingleInstance() {
    static PromiseEventFilter promiseEventFilter;
    return promiseEventFilter;
}

bool PromiseEventFilter::eventFilter(QObject *object, QEvent *event) {
    std::pair<QObject *, QEvent::Type> key = { object, event->type() };

    eventFilterRecursiveCount_.fetch_add(1);
    std::list<Listeners::iterator> itrs;
    for (Listeners::iterator itr = listeners_.lower_bound(key);
        itr != listeners_.end() && key == itr->first; ++itr) {
        itrs.push_back(itr);
    }
    for (Listeners::iterator itr : itrs) {
        itr->second(object, event);
    }

    int value = eventFilterRecursiveCount_.fetch_sub(1);
    if (value <= 1) {
        for (Listeners::iterator itr : removeLaters_) {
            listeners_.erase(itr);
        }
        removeLaters_.clear();
    }

    return QObject::eventFilter(object, event);
}

// Wait event will wait the event for only once
Promise waitEvent(QObject *object,
                  QEvent::Type  eventType,
                  bool          callSysHandler) {
    auto listener = std::make_shared<PromiseEventFilter::Listeners::iterator>();
    Promise promise = newPromise([listener, object, eventType, callSysHandler](Defer &defer) {

        std::shared_ptr<bool> disableFilter = std::make_shared<bool>(false);
        //PTI;
        *listener = PromiseEventFilter::getSingleInstance().addEventListener(
            object, eventType, [defer, callSysHandler, disableFilter](QObject *object, QEvent *event) {
            (void)object;
            if (event->type() == QEvent::Destroy) {
                defer.reject();
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
                defer.resolve(event);
                return true;
            }
            else {
                //PTI;
                defer.resolve(event);
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

Promise QtTimerHolder::delay(int time_ms) {
    int timerId = getInstance().startTimer(time_ms);

    return newPromise([timerId](Defer &defer) {
        getInstance().defers_.insert({ timerId, defer });
    }).finally([timerId]() {
        getInstance().killTimer(timerId);
        getInstance().defers_.erase(timerId);
    });
}

Promise QtTimerHolder::yield() {
    return delay(0);
}

Promise QtTimerHolder::setTimeout(const std::function<void(bool)> &func,
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
        Defer &defer = found->second;
        defer.resolve();
    }
    QObject::timerEvent(event);
}

QtTimerHolder &QtTimerHolder::getInstance() {
    static QtTimerHolder s_qtTimerHolder_;
    return s_qtTimerHolder_;
}

Promise delay(int time_ms) {
    return QtTimerHolder::delay(time_ms);
}

Promise yield() {
    return QtTimerHolder::yield();
}

Promise setTimeout(const std::function<void(bool)> &func,
                 int time_ms) {
    return QtTimerHolder::setTimeout(func, time_ms);
}


void cancelDelay(Promise promise) {
    promise.reject();
}

void clearTimeout(Promise promise) {
    cancelDelay(promise);
}

}

#endif
