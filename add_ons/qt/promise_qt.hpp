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

#include "../../include/promise.hpp"
#include <chrono>
#include <QObject>
#include <QTimerEvent>
#include <QApplication>

namespace promise {


class PromiseEventFilter : public QObject {
private:
    PROMISE_API PromiseEventFilter();

public:
    using Listener = std::function<bool(QObject *, QEvent *)>;
    using Listeners = std::multimap<std::pair<QObject *, QEvent::Type>, Listener>;

    PROMISE_API Listeners::iterator addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func);
    PROMISE_API void removeEventListener(Listeners::iterator itr);
    PROMISE_API static PromiseEventFilter &getSingleInstance();

protected:
    PROMISE_API bool eventFilter(QObject *object, QEvent *event) override;
    PROMISE_API bool removeObjectFilters(QObject *object);
    Listeners listeners_;
};

// Wait event will wait the event for only once
PROMISE_API Promise waitEvent(QObject      *object,
                              QEvent::Type  eventType,
                              bool          callSysHandler = false);

struct QtTimerHolder: QObject {
    PROMISE_API ~QtTimerHolder();
private:
    PROMISE_API QtTimerHolder();
public:
    PROMISE_API static Promise delay(int time_ms);
    PROMISE_API static Promise yield();
    PROMISE_API static Promise setTimeout(const std::function<void(bool)> &func,
                              int time_ms);

protected:
    PROMISE_API void timerEvent(QTimerEvent *event);
private:
    std::map<int, promise::Defer>  defers_;

    PROMISE_API static QtTimerHolder &getInstance();
};


PROMISE_API Promise delay(int time_ms);
PROMISE_API Promise yield();
PROMISE_API Promise setTimeout(const std::function<void(bool)> &func,
                               int time_ms);
PROMISE_API void cancelDelay(Promise promise);
PROMISE_API void clearTimeout(Promise promise);

}


#ifdef PROMISE_HEADONLY
#include "promise_qt.cpp"
#endif

#endif
