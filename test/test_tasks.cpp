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
#include "timer.hpp"
#include <boost/chrono.hpp>

using namespace promise;
using namespace std;
using namespace boost::asio;
using namespace boost::chrono;

static const int N = 1000000;

void dump(string name, int n,
    steady_clock::time_point start,
    steady_clock::time_point end)
{
    cout << name << "    " << n << "      " << 
        (end - start) / n << "/op" << endl;
}

void task(io_service &io, int task_id, int count, int *pcoro, Defer d) {
    if (count == 0) {
        -- *pcoro;
        if (*pcoro == 0)
            d.resolve();
        return;
    }

    yield(io).then([=, &io]() {
        task(io, task_id, count - 1, pcoro, d);
    });
};


Defer test_switch(io_service &io, int coro)
{
    steady_clock::time_point start = steady_clock::now();

    int *pcoro = new int(coro);

    return newPromise([=, &io](Defer d){
        for (int task_id = 0; task_id < coro; ++task_id) {
            task(io, task_id, N / coro, pcoro, d);
        }
    }).then([=](){
        delete pcoro;
        steady_clock::time_point end = steady_clock::now();
        dump("BenchmarkSwitch_" + std::to_string(coro), N, start, end);
    });
}

int main() {
    io_service io;

    While([&](Defer d) {
        printf("In while ...\n");
        test_switch(io, 1).then([&]() {
            return test_switch(io, 1000);
        }).then([&]() {
            return test_switch(io, 10000);
        }).then([&]() {
            return test_switch(io, 100000);
        }).then(d);
    });

    io.run();
    return 0;
}
