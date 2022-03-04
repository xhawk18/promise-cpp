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

    return newPromise(PM_LOC, [](Defer d) {
        output_func_name();
        d.resolve(PM_LOC, 3, 5, 6);
    }).then(PM_LOC, [](pm_any &any){
        output_func_name();
        return resolve(PM_LOC, 3, 5, 16);
    }).then(PM_LOC, [](const int &a, int b, int c) {
        printf("%d %d %d\n", a, b, c);
        output_func_name();
    }).then(PM_LOC, [](){
        output_func_name();
    }).then(PM_LOC, [&next](){
        output_func_name();
        next = newPromise(PM_LOC, [](Defer d) {
            output_func_name();
        });
        //Will call next.resole() or next.reject() later
        //throw 33;
        //next.reject(55, 66);
        return next;
    }).finally(PM_LOC, [](int n) {
        printf("finally n == %d\n", n);
    }).then(PM_LOC, [](int n, char c) {
        output_func_name();
        printf("n = %d, c = %c\n", (int)n, c);
    }).then(PM_LOC, [](char n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail(PM_LOC, [](char n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail(PM_LOC, [](short n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail(PM_LOC, [](int &n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).fail(PM_LOC, [](const std::string &str) {
        output_func_name();
        printf("str = %s\n", str.c_str());
    }).fail(PM_LOC, [](uint64_t n) {
        output_func_name();
        printf("n = %d\n", (int)n);
    }).then(PM_LOC, test1)
    .then(PM_LOC, test2)
    .then(PM_LOC, &test3);
    //.always([]() {
    //    output_func_name();
    //});
}

void test_promise_all() {
    std::vector<Promise> ds = {
        newPromise(PM_LOC, [](Defer d) {
            output_func_name();
            d.resolve(PM_LOC, 1);
            //d.reject(1);
        }),
        newPromise(PM_LOC, [](Defer d) {
            output_func_name();
            d.resolve(PM_LOC, 2);
        })
    };

    all(PM_LOC, ds).then(PM_LOC, []() {
        output_func_name();
    }, []() {
        output_func_name();
    });
}

int main(int argc, char **argv) {
    handleUncaughtException([](Promise &d) {
       
        d.fail(PM_LOC, [](long n, int m) {
            printf("UncaughtException parameters = %d %d\n", (int)n, m);
        }).fail(PM_LOC, []() {
            printf("UncaughtException\n");
        });
    });

    test_promise_all();

    Promise next;

    run(next);
    printf("======  after call run ======\n");

    //next.reject(123, 'a');
    if(next) {
        next.reject(PM_LOC, (long)123, (int)345);   //it will go to handleUncaughtException(...)
    }
    //next.resolve('b');
    //next.reject(std::string("xhchen"));
    //next.reject(45);

    return 0;
}
