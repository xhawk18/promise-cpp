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
#include "uv.h"
#include <stdio.h>
#include "timer.hpp"
using namespace promise;

void testTimer() {
    delay(500).then([]() {
        printf("In then 1\n");
        return delay(1000);
    }).then([]() {
        printf("In then 2\n");
        return delay(2000);
    }).then([]() {
        printf("In then 3\n");
        return delay(3000);
    }).then([]() {
        printf("In last then\n");
    });
}

Defer testPerformance() {
    static int i = 0;
    static uint64_t count = 0;
    static uint64_t hrtime;

    if (count == 0) {
        hrtime = uv_hrtime();
    }

    uint64_t interval = uv_hrtime() - hrtime;
    if(interval >= 2e9) {
        printf("time = %u, %u per seconds\n", (int)interval, (int)(uint64_t)(count / (interval / 1e9)));
        count = 0;
        hrtime = uv_hrtime();
    }

    ++count;
    return newPromise([](Defer d) {
        ++i;
        d.resolve(12);
    }).then([](int n) {
        ++i;
        return '3';
    }).then([](char ch) {
        ++i;
    }).then([]() {
        ++i;
        return newPromise([](Defer d) {
            ++i;
            d.reject(66);
        });
    }).fail([](int n) {
        ++i;
    }).always([]() {
        ++i;
        delay(0).then([]() {
            testPerformance();
        });
    });
}

void testWhile(int loop, uint64_t delay_ms){
    std::shared_ptr<int> i(new int(loop));
    While([=](Defer d){
        if((*i)-- <= 0){
            d.reject();
            return;
        }

        printf("in %s, i = %d -> %d, delay_ms = %u\n", __func__, *i, loop, (uint32_t)delay_ms);
        delay(delay_ms).then([=](){
            d.resolve();
        });
    }).always([=](){
        printf("loop = %d, delay_ms = %u, over!\n", loop, (uint32_t)delay_ms);
    });
}

Defer testTimer3() {
    printf("in testTimer3, line %d\n", __LINE__);
    return setTimeout(2000).then([]() {
        printf("in testTimer3, line %d\n", __LINE__);
        return setTimeout(2000);
    }).then([]() {
        printf("in testTimer3, line %d\n", __LINE__);
        return setTimeout(2000);
    }).then([]() {
        printf("in testTimer3, line %d\n", __LINE__);
    });
}

void testTimer2() {
    printf("in testTimer2 %d\n");
    setTimeout(1000).then([]() {
        return testTimer3();
    });
}

int main() {
    uv_loop_t *loop = uv_default_loop();

    testTimer2();
    testTimer();
    testPerformance();
    
    testWhile(10, 500);
    testWhile(30, 100);
    testWhile(5, 2000);

    return uv_run(loop, UV_RUN_DEFAULT);
}
