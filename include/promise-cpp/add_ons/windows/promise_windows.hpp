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
#ifndef INC_PROMISE_WIN32_HPP_
#define INC_PROMISE_WIN32_HPP_

//
// Promisified timer based on promise-cpp on windows platform
// It can be used in any windows application other than MFC
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
#include <map>
#include <windows.h>

namespace promise {

inline void cancelDelay(Defer d);
inline void clearTimeout(Defer d);

struct WindowsTimerHolder {
    WindowsTimerHolder() {};
public:
    static Promise delay(int time_ms) {
        UINT_PTR timerId = ::SetTimer(NULL, 0, (UINT)time_ms, &WindowsTimerHolder::timerEvent);

        return newPromise([timerId](Defer &Defer) {
            //printf("timerId = %d\n", (int)timerId);
            WindowsTimerHolder::getDefers().insert({ timerId, Defer });
        }).then([timerId]() {
            WindowsTimerHolder::getDefers().erase(timerId);
            return promise::resolve();
        }, [timerId]() {
            ::KillTimer(NULL, timerId);
            WindowsTimerHolder::getDefers().erase(timerId);
            return promise::reject();
        });

    }

    static Promise yield() {
        return delay(0);
    }

    static Promise setTimeout(const std::function<void(bool)> &func,
                              int time_ms) {
        return delay(time_ms).then([func]() {
            func(false);
        }, [func]() {
            func(true);
        });
    }

protected:
    static void CALLBACK timerEvent(HWND unnamedParam1,
                                    UINT unnamedParam2,
                                    UINT_PTR timerId,
                                    DWORD unnamedParam4) {
        (void)unnamedParam1;
        (void)unnamedParam2;
        (void)unnamedParam4;
        //printf("%d %d %d %d\n", (int)unnamedParam1, (int)unnamedParam2, (int)unnamedParam3, (int)unnamedParam4);
        auto found = getDefers().find(timerId);
        if (found != getDefers().end()) {
            Defer d = found->second;
            d.resolve();
        }
    }

private:
    friend void cancelDelay(Defer d);

    inline static std::map<UINT_PTR, promise::Defer> &getDefers() {
        static std::map<UINT_PTR, promise::Defer>  s_defers_;
        return s_defers_;
    }
};

inline Promise delay(int time_ms) {
    return WindowsTimerHolder::delay(time_ms);
}

inline Promise yield() {
    return WindowsTimerHolder::yield();
}

inline Promise setTimeout(const std::function<void(bool)> &func,
    int time_ms) {
    return WindowsTimerHolder::setTimeout(func, time_ms);
}


inline void cancelDelay(Defer d) {
    d.reject();
}

inline void clearTimeout(Defer d) {
    cancelDelay(d);
}

}
#endif
