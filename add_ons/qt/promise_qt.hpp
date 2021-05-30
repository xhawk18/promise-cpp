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
// Promisified timer based on promise-cpp and boost::asio
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

inline void cancelDelay(Defer d);
inline void clearTimeout(Defer d);

struct QtTimerHolder: QObject {
    QtTimerHolder() {};
public:
    Defer delay(int time_ms) {
        return newPromise([time_ms, this](Defer &d) {
            int timerId = this->startTimer(time_ms);

            QtTimer *timer = pm_new<QtTimer>();
            timer->timerId_ = timerId;
            timer->timerHolder_ = this;
            d->any_ = timer;
            this->defers_.insert({ timerId, d });
            });
    }

    Defer yield() {
        return delay(0);
    }

    Defer setTimeout(const std::function<void(bool)> &func,
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
            clearQtTimer(d);
            d.resolve();
        }
        QObject::timerEvent(event);
    }
private:
    struct QtTimer {
        int            timerId_;
        QtTimerHolder *timerHolder_;
    };

    friend void cancelDelay(Defer d);
    static void clearQtTimer(Defer &d) {
        if (!d->any_.empty()) {
            QtTimer **ptimer = any_cast<QtTimer *>(&d->any_);
            d->any_.clear();
            if (ptimer != nullptr) {
                QtTimer *timer = *ptimer;
                timer->timerHolder_->defers_.erase(timer->timerId_);
                timer->timerHolder_->killTimer(timer->timerId_);
                pm_delete(timer);
            }
        }
    }

    std::map<int, promise::Defer>  defers_;
};


inline void cancelDelay(Defer d) {
    d = d.find_pending();
    if (d.operator->()) {
        QtTimerHolder::clearQtTimer(d);
        d.reject();
    }
}

inline void clearTimeout(Defer d) {
    cancelDelay(d);
}

}
#endif
