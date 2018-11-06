/*
 * Promise API implemented by cpp as Javascript promise style 
 *
 * Copyright (c) 2016, xhawk18
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
#ifndef INC_SIMPLE_TASK_HPP_
#define INC_SIMPLE_TASK_HPP_


#include <string>
#include <map>
#include <list>
#include <deque>
#include <chrono>
#include <thread>
#include <utility>
#include "promise.hpp"


class Service {
    using Defer     = promise::Defer;
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    using Timers    = std::multimap<TimePoint, Defer>;
    using Tasks     = std::deque<Defer>;

    Timers timers_;
    Tasks  tasks_;

public:
    // delay for milliseconds
    Defer delay(uint64_t time_ms) {
        return promise::newPromise([&](Defer d) {
            TimePoint now = std::chrono::steady_clock::now();
            TimePoint time = now + std::chrono::milliseconds(time_ms);
            timers_.emplace(time, d);
        });
    }

    // yield for other tasks to run
    Defer yield() {
        return promise::newPromise([&](Defer d) {
            return tasks_.push_back(d);
        });
    }

    // run the service loop
    void run() {
        while(tasks_.size() > 0 || timers_.size() > 0){
            while(timers_.size() > 0){
                TimePoint now = std::chrono::steady_clock::now();
                TimePoint time = timers_.begin()->first;
                if(time <= now){
                    Defer d = timers_.begin()->second;
                    tasks_.push_back(d);
                    timers_.erase(timers_.begin());
                }
                else if(tasks_.size() == 0)
                    std::this_thread::sleep_for(time - now);
                else
                    break;
            }

            // Check fixed size of tasks in this loop, so that timer have a chance to run.
            size_t size = tasks_.size();
            for(size_t i = 0; i < size; ++i){
                Defer d = tasks_.front();
                tasks_.pop_front();
                d.resolve();
            }
        }
   }
};

#endif

