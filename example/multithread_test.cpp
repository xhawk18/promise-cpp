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


#include <stdio.h>
#include <iostream>
#include <string>
#include <chrono>
#include <promise-cpp/promise.hpp>
#include <promise-cpp/add_ons/simple_task/simple_task.hpp>

using namespace promise;
namespace chrono       = std::chrono;
using     steady_clock = std::chrono::steady_clock;

static const int N = 10000;

void dump(std::string name, int n,
    steady_clock::time_point start,
    steady_clock::time_point end)
{
    auto ns = chrono::duration_cast<chrono::nanoseconds>(end - start);
    std::cout << name << "    " << n << "      " <<
        ns.count() / n << 
        "ns/op" << std::endl;
}

void task(Service &io, int task_id, int count, int *pcoro, Defer defer) {
    if (count == 0) {
        -- *pcoro;
        if (*pcoro == 0)
            defer.resolve();
        return;
    }

    io.yield().then([=, &io]() {
        task(io, task_id, count - 1, pcoro, defer);
    });
};


Promise test_switch(Service &io, int coro) {
    steady_clock::time_point start = steady_clock::now();

    int *pcoro = new int(coro);

    return newPromise([=, &io](Defer &defer){
        for (int task_id = 0; task_id < coro; ++task_id) {
            task(io, task_id, N / coro, pcoro, defer);
        }
    }).then([=]() {
        delete pcoro;
        steady_clock::time_point end = steady_clock::now();
        dump("BenchmarkSwitch_" + std::to_string(coro), N, start, end);
    });
}


int main() {
    Service io;

    std::function<void()> func = []() {
    };
    promise::any ff = [func]() {
        func();
    };

    promise::newPromise([=](Defer &defer) {
    }).then(ff);
    //return 0;

    int i = 0;
    doWhile([&](DeferLoop &loop) {
#ifdef PM_DEBUG
        printf("In while ..., alloc_size = %d\n", (int)(*dbg_alloc_size()));
#else
        printf("In while ...\n");
#endif
        //Sleep(5000);
        test_switch(io, 1).then([&]() {
            return test_switch(io, 10);
        }).then([&]() {
            return test_switch(io, 100);
        }).then([&]() {

            io.setAutoStop(false);
            // Run in new thread
            return newPromise([](Defer &defer) {
                std::thread([=]() {
                    defer.resolve();
                }).detach();

            });

        }).then([&]() {
            printf("after thread\n");

            // Run in io thread
            return newPromise([&](Defer &defer) {
                io.runInIoThread([=]() {
                    defer.resolve();
                });
            });

        }).then([&]() {
            io.setAutoStop(true);

            return test_switch(io, 1000);
        }).then([&, loop]() {
            if (i++ > 3) loop.doBreak();
            return test_switch(io, 10000);
        }).then(loop);
    });

    io.run();
    return 0;
}
