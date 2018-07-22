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
#include <string>
#include <map>
#include <chrono>
#include <thread>
#include <utility>
#include "promise.hpp"

using namespace promise;
using namespace std;


typedef chrono::time_point<chrono::steady_clock> TimePoint;
typedef multimap<TimePoint, Defer> Service;

Defer delay(Service &service, uint64_t time_ms) {
    return newPromise([&](Defer d){
        TimePoint now = chrono::steady_clock::now();
        TimePoint time = now + std::chrono::milliseconds(time_ms);
        service.emplace(time, d);
    });
}

void run(Service &service){
    while(service.size() > 0){
        TimePoint now = chrono::steady_clock::now();
        TimePoint time = service.begin()->first;
        if(time <= now){
            Defer d = service.begin()->second;
            d.resolve();
            service.erase(service.begin());
        }
        else 
            this_thread::sleep_for(time - now);
    }
}


Defer testTimer(Service &service) {

    return delay(service, 3000).then([&] {
        printf("timer after 3000 ms!\n");
        return delay(service, 1000);
    }).then([&] {
        printf("timer after 1000 ms!\n");
        return delay(service, 12000);
    }).then([] {
        printf("timer after 2000 ms!\n");
    }).fail([] {
        printf("timer cancelled!\n");
    });
}


#include <iostream>

int main() {
    

    Service service;

    testTimer(service);
    testTimer(service);
    testTimer(service);

    run(service);
    return 0;
}
