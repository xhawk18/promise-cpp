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
#include <stdexcept>
#include <promise-cpp/promise.hpp>


class Service {
    using Defer     = promise::Defer;
    using Promise   = promise::Promise;
    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    using Timers    = std::multimap<TimePoint, Defer>;
    using Tasks     = std::deque<Defer>;
#if PROMISE_MULTITHREAD
    using Mutex     = promise::Mutex;
#endif

    Timers timers_;
    Tasks  tasks_;
#if PROMISE_MULTITHREAD
    //std::recursive_mutex mutex_;
    std::shared_ptr<Mutex> mutex_;
#endif
    std::condition_variable_any cond_;
    std::atomic<bool> isAutoStop_;
    std::atomic<bool> isStop_;
    //Unlock and then lock
#if PROMISE_MULTITHREAD
    struct unlock_guard_t {
        inline unlock_guard_t(std::shared_ptr<Mutex> mutex)
            : mutex_(mutex)
            , lock_count_(mutex->lock_count()) {
            mutex_->unlock(lock_count_);
        }
        inline ~unlock_guard_t() {
            mutex_->lock(lock_count_);
        }
        std::shared_ptr<Mutex> mutex_;
        size_t lock_count_;
    };
#endif
public:
    Service()
        : isAutoStop_(true)
        , isStop_(false)
#if PROMISE_MULTITHREAD
        , mutex_(std::make_shared<Mutex>())
#endif
    {
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
#if PROMISE_MULTITHREAD
            std::lock_guard<Mutex> lock(*mutex_);
#endif
            tasks_.push_back(defer);
            cond_.notify_one();
        });
    }

    // Resolve the defer object in this io thread
    void runInIoThread(const std::function<void()> &func) {
        promise::newPromise([=](Defer &defer) {
#if PROMISE_MULTITHREAD
            std::lock_guard<Mutex> lock(*mutex_);
#endif
            tasks_.push_back(defer);
            cond_.notify_one();
        }).then([func]() {
            func();
        });
    }

    // Set if the io thread will auto exist if no waiting tasks and timers.
    void setAutoStop(bool isAutoExit) {
#if PROMISE_MULTITHREAD
        std::lock_guard<Mutex> lock(*mutex_);
#endif
        isAutoStop_ = isAutoExit;
        cond_.notify_one();
    }


    // run the service loop
    void run() {
#if PROMISE_MULTITHREAD
        std::unique_lock<Mutex> lock(*mutex_);
#endif

        while(!isStop_ && (!isAutoStop_ || tasks_.size() > 0 || timers_.size() > 0)) {

            if (tasks_.size() == 0 && timers_.size() == 0) {
                cond_.wait(lock);
                continue;
            }

            while (!isStop_ && timers_.size() > 0) {
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
            if(!isStop_ && tasks_.size() > 0) {
                size_t size = tasks_.size();
                for(size_t i = 0; i < size; ++i){
                    Defer defer = tasks_.front();
                    tasks_.pop_front();

#if PROMISE_MULTITHREAD
                    unlock_guard_t unlock(mutex_);
                    defer.resolve();
                    break; // for only once
#else
                    defer.resolve(); // loop with the size
#endif
                }
            }
        }

        // Clear pending timers and tasks
        while (timers_.size() > 0 || tasks_.size()) {
            while (timers_.size() > 0) {
                Defer defer = timers_.begin()->second;
                timers_.erase(timers_.begin());
#if PROMISE_MULTITHREAD
                unlock_guard_t unlock(mutex_);
#endif
                defer.reject(std::runtime_error("service stopped"));
            }
            while (tasks_.size() > 0) {
                Defer defer = tasks_.front();
                tasks_.pop_front();
#if PROMISE_MULTITHREAD
                unlock_guard_t unlock(mutex_);
#endif
                defer.reject(std::runtime_error("service stopped"));
            }
        }
    }

    // stop the service loop
    void stop() {
#if PROMISE_MULTITHREAD
        std::lock_guard<Mutex> lock(*mutex_);
#endif
        isStop_ = true;
        cond_.notify_one();
    }
};

#endif

