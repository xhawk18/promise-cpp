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
#include "promise.hpp"

using namespace promise;

void test1() {
    printf("in function %s, line %d\n", __func__, __LINE__);
}
int test2() {
    printf("in function %s, line %d\n", __func__, __LINE__);
    return 5;
}
void test3(int n) {
    printf("in function %s, line %d\n", __func__, __LINE__);
    printf("n = %d\n", n);
}

int main(int argc, char **argv) {
    Defer d1;

    newPromise([argc](Defer d){
        printf("in function %s, line %d\n", __func__, __LINE__);
        d->resolve();
    }).then([]() {
        printf("in function %s, line %d\n", __func__, __LINE__);
        throw 33;
    }).then([](){
        printf("in function %s, line %d\n", __func__, __LINE__);
    }).then([&d1](){
        printf("in function %s, line %d\n", __func__, __LINE__);
        d1 = newPromise([](Defer d) {
            printf("in function %s, line %d\n", __func__, __LINE__);
            //d->resolve();
        });
        return d1;
    }).fail([](char n){
        printf("in function %s, line %d, failed with value %d\n", __func__, __LINE__, n);
    }).fail([](short n) {
        printf("in function %s, line %d, failed with value %d\n", __func__, __LINE__, n);
    }).fail([](int &n) {
        printf("in function %s, line %d, failed with value %d\n", __func__, __LINE__, n);
    }).fail([](uint64_t n) {
        printf("in function %s, line %d, failed with value %lld\n", __func__, __LINE__, n);
    }).then(test1)
    .then(test2)
    .then(test3)
    .always([]() {
        printf("in function %s, line %d\n", __func__, __LINE__);
    });

    printf("call later=================\n");
    //d1.resolve();
    //d1.reject(123);

    return 0;
}

