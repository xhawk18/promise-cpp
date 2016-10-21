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
		printf("time = %lld, %lld per seconds\n", interval, (uint64_t)(count / (interval / 1e9)));
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

int main() {
    uv_loop_t *loop = uv_default_loop();

	testTimer();
	testPerformance();

    return uv_run(loop, UV_RUN_DEFAULT);
}
