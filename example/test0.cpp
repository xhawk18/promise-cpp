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
#include <vector>
#include "promise-cpp/promise.hpp"

using namespace promise;

#define output_func_name() do{ printf("in function %s, line %d\n", __func__, __LINE__); } while(0)

void test1() {
    output_func_name();
}

int test2() {
    output_func_name();
    return 5;
}

void test3(int n) {
    output_func_name();
    printf("n = %d\n", n);
}

Promise run(Promise &next){

    return newPromise([](Defer d) {
        output_func_name();
        d.resolve(3, 5, 6);
    }).then([](pm_any &any){
        output_func_name();
        return resolve(3, 5, 16);
    }).then([](const int &a, int b, int c) {
        printf("%d %d %d\n", a, b, c);
        output_func_name();
    }).then([](){
        output_func_name();
    }).then([&next](){
        output_func_name();
        next = newPromise([](Defer d) {
            output_func_name();
        });
        //Will call next.resole() or next.reject() later
        //throw 33;
        //next.reject(55, 66);
        return next;
    }).finally([](int n) {
        printf("finally n == %d\n", n);
    }).then([](int n, char c) {
        output_func_name();
        printf("n = %d, c = %c\n", (int)n, c);
    }).then([](char n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](char n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](short n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](int &n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail([](const std::string &str) {
        output_func_name();
        printf("str = %s\n", str.c_str());
    }).fail([](uint64_t n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).then(test1)
    .then(test2)
    .then(&test3);
    //.always([]() {
    //    output_func_name();
    //});
}

void test_promise_all() {
    std::vector<Promise> ds = {
        newPromise([](Defer d) {
            output_func_name();
            d.resolve(1);
            //d.reject(1);
        }),
        newPromise([](Defer d) {
            output_func_name();
            d.resolve(2);
        })
    };

    all(ds).then([]() {
        output_func_name();
    }, []() {
        output_func_name();
    });
}

int main(int argc, char **argv) {
    handleUncaughtException([](Promise &d) {
       
        d.fail([](long n, int m) {
            printf("UncaughtException parameters = %d %d\n", (int)n, m);
        }).fail([]() {
            printf("UncaughtException\n");
        });
    });

    test_promise_all();

    Promise next;

    run(next);
    printf("======  after call run ======\n");

    //next.reject(123, 'a');
    if(next) {
        next.reject((long)123, (int)345);   //it will go to handleUncaughtException(...)
    }
    //next.resolve('b');
    //next.reject(std::string("xhchen"));
    //next.reject(45);

    return 0;
}
