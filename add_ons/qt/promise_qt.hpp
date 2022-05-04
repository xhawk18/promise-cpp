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

#include "promise-cpp/promise.hpp"
#include <chrono>
#include <set>
#include <atomic>
#include <QObject>
#include <QTimerEvent>
#include <QApplication>

#ifdef PROMISE_HEADONLY
#define PROMISE_QT_API inline
#elif defined PROMISE_BUILD_SHARED

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(promise_qt_EXPORTS) // add by CMake 
#    ifdef __GNUC__
#      define  PROMISE_QT_API __attribute__(dllexport)
#    else
#      define  PROMISE_QT_API __declspec(dllexport)
#    endif
#  else
#    ifdef __GNUC__
#      define  PROMISE_QT_API __attribute__(dllimport)
#    else
#      define  PROMISE_QT_API __declspec(dllimport)
#    endif
#  endif // promise_qt_EXPORTS

#elif defined __GNUC__
#  if __GNUC__ >= 4
#    define PROMISE_QT_API __attribute__ ((visibility ("default")))
#  else
#    define PROMISE_QT_API
#  endif

#elif defined __clang__
#  define PROMISE_QT_API __attribute__ ((visibility ("default")))
#else
#   error "Do not know how to export classes for this platform"
#endif

#else
#define PROMISE_QT_API
#endif

namespace promise {

class PromiseEventListener;
class PromiseEventPrivate;
class PromiseEventFilter : public QObject {
private:
    PROMISE_QT_API PromiseEventFilter();

public:

    PROMISE_QT_API std::weak_ptr<PromiseEventListener> addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func);
    PROMISE_QT_API void removeEventListener(std::weak_ptr<PromiseEventListener> listener);
    PROMISE_QT_API static PromiseEventFilter &getSingleInstance();

protected:
    PROMISE_QT_API bool eventFilter(QObject *object, QEvent *event) override;
    std::shared_ptr<PromiseEventPrivate> private_;
};

// Wait event will wait the event for only once
PROMISE_QT_API Promise waitEvent(QObject      *object,
                                 QEvent::Type  eventType,
                                 bool          callSysHandler = false);
PROMISE_QT_API std::weak_ptr<PromiseEventListener> addEventListener(QObject *object, QEvent::Type eventType, const std::function<bool(QObject *, QEvent *)> &func);
PROMISE_QT_API void removeEventListener(std::weak_ptr<PromiseEventListener> listener);

struct QtTimerHolder: QObject {
    PROMISE_QT_API ~QtTimerHolder();
private:
    PROMISE_QT_API QtTimerHolder();
public:
    PROMISE_QT_API static Promise delay(int time_ms);
    PROMISE_QT_API static Promise yield();
    PROMISE_QT_API static Promise setTimeout(const std::function<void(bool)> &func,
                                             int time_ms);

protected:
    PROMISE_QT_API void timerEvent(QTimerEvent *event);
private:
    std::map<int, promise::Defer>  defers_;

    PROMISE_QT_API static QtTimerHolder &getInstance();
};


PROMISE_QT_API Promise delay(int time_ms);
PROMISE_QT_API Promise yield();
PROMISE_QT_API Promise setTimeout(const std::function<void(bool)> &func,
                                  int time_ms);
PROMISE_QT_API void cancelDelay(Promise promise);
PROMISE_QT_API void clearTimeout(Promise promise);


// Low level set timeout
struct PROMISE_QT_API QtPromiseTimerHandler {
    void wait();
    QTimer *timer_;
};
PROMISE_QT_API std::shared_ptr<QtPromiseTimerHandler> qtPromiseSetTimeout(const std::function<void()> &cb, int ms);

}


#ifdef PROMISE_HEADONLY
#include "promise_qt_inl.hpp"
#endif

#endif
