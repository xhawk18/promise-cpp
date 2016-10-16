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
#include "promise.hpp"

using namespace promise;

#define PRINT_FUNC_INFO() do{ printf("in function %s, line %d\n", __func__, __LINE__); } while(0)

void test1() {
    PRINT_FUNC_INFO();
}

int test2() {
    PRINT_FUNC_INFO();
    return 5;
}

void test3(int n) {
    PRINT_FUNC_INFO();
    printf("n = %d\n", n);
}

Defer run(Defer &next){
    return newPromise([](Defer d){
        PRINT_FUNC_INFO();
        d->resolve();
    }).then([]() {
        PRINT_FUNC_INFO();
        //throw 33;
    }).then([](){
        PRINT_FUNC_INFO();
    }).then([&next](){
        PRINT_FUNC_INFO();
        next = newPromise([](Defer d) {
            PRINT_FUNC_INFO();
        });
        //Will call next.resole() or next.reject() later
        return next;
    }).then([](int n) {
        PRINT_FUNC_INFO();
        printf("n = %d\n", (int)n);
    }).fail([](char n){
        PRINT_FUNC_INFO();
        printf("n = %d\n", (int)n);
    }).fail([](short n) {
        PRINT_FUNC_INFO();
        printf("n = %d\n", (int)n);
    }).fail([](int &n) {
        PRINT_FUNC_INFO();
        printf("n = %d\n", (int)n);
    }).fail([](const std::string &str) {
        PRINT_FUNC_INFO();
        printf("str = %s\n", str.c_str());
    }).fail([](uint64_t n) {
        PRINT_FUNC_INFO();
        printf("n = %d\n", (int)n);
    }).then(test1)
    .then(test2)
    .then(test3)
    .always([]() {
        PRINT_FUNC_INFO();
    });
}

int main(int argc, char **argv) {
    Defer next;

    run(next);
    printf("======  after call run ======\n");

    next.resolve(123);
    //next.reject('c');
    //next.reject(std::string("xhchen"));
    //next.reject(45);

    return 0;
}
