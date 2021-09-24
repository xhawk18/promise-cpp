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
    PromiseEventFilter();

public:
    using Listener = std::function<bool(QObject *, QEvent *)>;
    using Listeners = std::multimap<std::pair<QObject *, QEvent::Type>, Listener>;

    Listeners::iterator addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func);
    void removeEventListener(Listeners::iterator itr);
    static PromiseEventFilter &getSingleInstance();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    bool removeObjectFilters(QObject *object);
    Listeners listeners_;
};

// Wait event will wait the event for only once
Defer waitEvent(QObject      *object,
                QEvent::Type  eventType,
                bool          callSysHandler = false);

struct QtTimerHolder: QObject {
    ~QtTimerHolder();
private:
    QtTimerHolder();
public:
    static Defer delay(int time_ms);
    static Defer yield();
    static Defer setTimeout(const std::function<void(bool)> &func,
                            int time_ms);

protected:
    void timerEvent(QTimerEvent *event);
private:
    std::map<int, promise::Callback>  defers_;

    static QtTimerHolder &getInstance();
};


Defer delay(int time_ms);
Defer yield();
Defer setTimeout(const std::function<void(bool)> &func,
                          int time_ms);
void cancelDelay(Defer d);
void clearTimeout(Defer d);

}
#endif
