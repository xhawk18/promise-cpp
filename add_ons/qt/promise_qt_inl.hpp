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
#include <QTimer>
#include <QThread>

namespace promise {

class PromiseEventListener {
public:
    using Listeners = std::multimap<std::pair<QObject *, QEvent::Type>, std::shared_ptr<PromiseEventListener>>;
    std::function<bool(QObject *, QEvent *)> cb_;
    Listeners::iterator self_;
};

class PromiseEventPrivate {
public:
    PromiseEventListener::Listeners listeners_;
};

PromiseEventFilter::PromiseEventFilter() 
    : private_(new PromiseEventPrivate) {
}

std::weak_ptr<PromiseEventListener> PromiseEventFilter::addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func) {
    std::shared_ptr<PromiseEventListener> listener = std::make_shared<PromiseEventListener>();
    std::pair<QObject *, QEvent::Type> key = { object, eventType };
    auto itr = private_->listeners_.insert({ key, listener });
    listener->cb_ = func;
    listener->self_ = itr;
    return listener;
}

void PromiseEventFilter::removeEventListener(std::weak_ptr<PromiseEventListener> listener) {
    auto sListener = listener.lock();
    if (sListener) {
        private_->listeners_.erase(sListener->self_);
    }
}

PromiseEventFilter &PromiseEventFilter::getSingleInstance() {
    static PromiseEventFilter promiseEventFilter;
    return promiseEventFilter;
}

bool PromiseEventFilter::eventFilter(QObject *object, QEvent *event) {
    std::pair<QObject *, QEvent::Type> key = { object, event->type() };

    bool filtered = false;

    std::list<std::weak_ptr<PromiseEventListener>> listeners;
    for (auto itr = private_->listeners_.lower_bound(key);
        itr != private_->listeners_.end() && key == itr->first; ++itr) {
        listeners.push_back(itr->second);
    }
    for (const auto &listener : listeners) {
        auto sListener = listener.lock();
        if (sListener) {
            bool res = sListener->cb_(object, event);
            if (res) filtered = true;
        }
    }

    if (filtered) return true;
    else return QObject::eventFilter(object, event);
}

// Wait event will wait the event for only once
Promise waitEvent(QObject *object,
                  QEvent::Type  eventType,
                  bool          callSysHandler) {
    auto listener = std::make_shared<std::weak_ptr<PromiseEventListener>>();
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
        });
    }).finally([listener]() {
        //PTI;
        PromiseEventFilter::getSingleInstance().removeEventListener(*listener);
    });

    return promise;
}


std::weak_ptr<PromiseEventListener> addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func) {
    return PromiseEventFilter::getSingleInstance().addEventListener(object, eventType, func);
}

void removeEventListener(std::weak_ptr<PromiseEventListener> listener) {
    PromiseEventFilter::getSingleInstance().removeEventListener(listener);
}


QtTimerHolder::QtTimerHolder() {
}

QtTimerHolder::~QtTimerHolder() {
}


void QtPromiseTimerHandler::wait() {
    while (timer_ != nullptr) {
        QCoreApplication::processEvents();
    }
}


std::shared_ptr<QtPromiseTimerHandler> qtPromiseSetTimeout(const std::function<void()> &cb, int ms) {
    //LOG_INFO("setTimeout({})", ms);
    std::shared_ptr<QtPromiseTimerHandler> handler(new QtPromiseTimerHandler);
    handler->timer_ = new QTimer();

    QMetaObject::Connection conneciton = QObject::connect(QThread::currentThread(), &QThread::finished, [handler]() {
        QTimer *timer = handler->timer_;
        if (timer != nullptr) {
            handler->timer_ = nullptr;
            timer->stop();
            timer->deleteLater();
        }
    });

    handler->timer_->singleShot(ms, [handler, cb, conneciton]() {
        cb();
        QObject::disconnect(conneciton);
        QTimer *timer = handler->timer_;
        if (timer != nullptr) {
            handler->timer_ = nullptr;
            timer->deleteLater();
        }
    });

    return handler;
}

Promise QtTimerHolder::delay(int time_ms) {
    return newPromise([time_ms](Defer &defer) {
        qtPromiseSetTimeout([defer] {
            defer.resolve();
        }, time_ms);
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
