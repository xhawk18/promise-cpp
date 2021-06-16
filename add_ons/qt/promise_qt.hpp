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

#pragma once
#ifndef INC_PROMISE_QT_HPP_
#define INC_PROMISE_QT_HPP_

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

#include "../../promise.hpp"
#include <chrono>
#include <QObject>
#include <QTimerEvent>

namespace promise {


class PromiseEventFilter : public QObject {
private:
    friend class Singleton<PromiseEventFilter>;
    PromiseEventFilter() {}

public:
    using Listener = std::function<bool(QObject *, QEvent *)>;
    using Listeners = std::multimap<std::pair<QObject *, QEvent::Type>, Listener>;

    Listeners::iterator addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func) {
        std::pair<QObject *, QEvent::Type> key = { object, eventType };
        return listeners_.insert({ key, func });
    }

    void removeEventListener(Listeners::iterator itr) {
        listeners_.erase(itr);
    }

    static inline PromiseEventFilter &getSingleInstance() {
        static PromiseEventFilter promiseEventFilter;
        return promiseEventFilter;
    }

protected:
    bool eventFilter(QObject *object, QEvent *event) {
        std::pair<QObject *, QEvent::Type> key = { object, event->type() };
        Listeners::iterator itr = listeners_.find(key);
        if (itr != listeners_.end()) {
            return itr->second(object, event);
        }

        if (event->type() == QEvent::Destroy) {
            return removeObjectFilters(object);
        }

        return false;
    }

    bool removeObjectFilters(QObject *object) {
        std::pair<QObject *, QEvent::Type> key = { object, QEvent::None };

        std::list<Listeners::iterator> deletes;
        
        for (Listeners::iterator itr = listeners_.lower_bound(key);
            itr != listeners_.end() && itr->first.first == object;
            ++itr) {
            deletes.push_back(itr);
        }

        for(Listeners::iterator itr: deletes) {
            itr->second(object, nullptr);
        }

        return false;
    }

    Listeners listeners_;
};

// Wait event will wait the event for only once
inline Defer waitEvent(QObject      *object,
                       QEvent::Type  eventType) {
    Defer promise = newPromise();

    auto listener = PromiseEventFilter::getSingleInstance().addEventListener(
        object, eventType, [promise](QObject *object, QEvent *event) {
            (void)object;
            if (event == nullptr)
                promise->reject();
            else
                promise->resolve(event);
            return false;
        }
    );

    promise.finally([listener]() {
        PromiseEventFilter::getSingleInstance().removeEventListener(listener);
    });
    
    return promise;
}


inline void cancelDelay(Defer d);
inline void clearTimeout(Defer d);

struct QtTimerHolder: QObject {
    QtTimerHolder() {
    };
    ~QtTimerHolder() {
    }
public:
    static Defer delay(int time_ms) {
        int timerId = getInstance().startTimer(time_ms);

        return newPromise([timerId](Defer &d) {
            getInstance().defers_.insert({ timerId, d });
        }).then([timerId]() {
            getInstance().defers_.erase(timerId);
            return promise::resolve();
        }, [timerId]() {
            getInstance().killTimer(timerId);
            getInstance().defers_.erase(timerId);
            return promise::reject();
        });
    }

    static Defer yield() {
        return delay(0);
    }

    static Defer setTimeout(const std::function<void(bool)> &func,
                            int time_ms) {
        return delay(time_ms).then([func]() {
            func(false);
        }, [func]() {
            func(true);
        });
    }

protected:
    void timerEvent(QTimerEvent *event) {
        int timerId = event->timerId();
        auto found = this->defers_.find(timerId);
        if (found != this->defers_.end()) {
            Defer d = found->second;
            d.resolve();
        }
        QObject::timerEvent(event);
    }
private:
    std::map<int, promise::Defer>  defers_;

    static QtTimerHolder &getInstance() {
        static QtTimerHolder s_qtTimerHolder_;
        return s_qtTimerHolder_;
    }
};


inline Defer delay(int time_ms) {
    return QtTimerHolder::delay(time_ms);
}

inline Defer yield() {
    return QtTimerHolder::yield();
}

inline Defer setTimeout(const std::function<void(bool)> &func,
                        int time_ms) {
    return QtTimerHolder::setTimeout(func, time_ms);
}


inline void cancelDelay(Defer d) {
    d.reject_pending();
}

inline void clearTimeout(Defer d) {
    cancelDelay(d);
}

}
#endif
