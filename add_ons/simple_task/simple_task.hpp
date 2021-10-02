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
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <utility>
#include "promise-cpp/promise.hpp"


class Service {
    using Defer     = promise::Defer;
    using Promise   = promise::Promise;
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    using Timers    = std::multimap<TimePoint, Defer>;
    using Tasks     = std::deque<Defer>;

    Timers timers_;
    Tasks  tasks_;
    std::recursive_mutex mutex_;
    std::condition_variable_any cond_;
    std::atomic<bool> isAutoExit_;

public:
    Service()
        : isAutoExit_(true) {
    }

    // delay for milliseconds
    Promise delay(uint64_t time_ms) {
        return promise::newPromise([&](Defer &defer) {
            TimePoint now = std::chrono::steady_clock::now();
            TimePoint time = now + std::chrono::milliseconds(time_ms);
            timers_.emplace(time, defer);
        });
    }

    // yield for other tasks to run
    Promise yield() {
        return promise::newPromise([&](Defer &defer) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            tasks_.push_back(defer);
            cond_.notify_one();
        });
    }

    // Resolve the defer object in this io thread
    void runInIoThread(const std::function<void()> &func) {
        promise::newPromise([=](Defer &defer) {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            tasks_.push_back(defer);
            cond_.notify_one();
        }).then([func]() {
            func();
        });
    }

    // Set if the io thread will auto exist if no waiting tasks and timers.
    void setAutoExit(bool isAutoExit) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        isAutoExit_ = isAutoExit;
        cond_.notify_one();
    }


    // run the service loop
    void run() {
        std::unique_lock<std::recursive_mutex> lock(mutex_);

        while(!isAutoExit_ || tasks_.size() > 0 || timers_.size() > 0){

            if (tasks_.size() == 0 && timers_.size() == 0) {
                cond_.wait(lock);
                continue;
            }

            while (timers_.size() > 0) {
                TimePoint now = std::chrono::steady_clock::now();
                TimePoint time = timers_.begin()->first;
                if (time <= now) {
                    Defer &defer = timers_.begin()->second;
                    tasks_.push_back(defer);
                    timers_.erase(timers_.begin());
                }
                else if (tasks_.size() == 0) {
                    //std::this_thread::sleep_for(time - now);
                    cond_.wait_for(lock, time - now);
                }
                else {
                    break;
                }
            }

            // Check fixed size of tasks in this loop, so that timer have a chance to run.
            if(tasks_.size() > 0) {
                size_t size = tasks_.size();
                for(size_t i = 0; i < size; ++i){
                    Defer defer = tasks_.front();
                    tasks_.pop_front();
                    defer.resolve();
                }
            }
        }
    }
};

#endif

