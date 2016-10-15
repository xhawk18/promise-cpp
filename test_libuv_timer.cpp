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
#include "../../promise.hpp"
using namespace promise;

void setTimeout(std::function<void()> cb, uint64_t timeout) {
    struct Handler : public uv_timer_t {
        std::function<void()> cb_;

        static void onTimer(uv_timer_t* timer) {
            Handler *handler = static_cast<Handler *>(timer);
            handler->cb_();

            delete handler;
        }

		void* operator new(size_t size){
			return allocator<Handler>::do_new(size);
		}

		void operator delete(void *ptr) {
			allocator<Handler>::do_delete(ptr);
		}
    };

    uv_loop_t *loop;
    Handler *handler = new Handler;
    uv_timer_t *timer = static_cast<uv_timer_t *>(handler);

    loop = uv_default_loop();
    handler->cb_ = cb;
    uv_timer_init(loop, timer);

    uv_timer_start(timer, &Handler::onTimer, timeout, 0);

}

Defer newDelay(uint64_t timeout) {
    return newPromise([timeout](Defer d) {
        setTimeout([d]() {
            d->resolve();
        }, timeout);
    });
}

void testTimer() {
	newPromise([](Defer d) {
		setTimeout([d]() {
			printf("In timerout 1\n");
			d.resolve(893);
		}, 1000);
	}).then([](const int &vv) {
		printf("In then 1, vv = %d\n", vv);
		return newDelay(1000);
	}).then([]() {
		printf("In then 2\n");
		return newDelay(2000);
	}).then([]() {
		printf("In then 3\n");
		return newDelay(3000);
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
		d->resolve(12);
	}).then([](int n) {
		++i;
		return '3';
	}).then([](char ch) {
		++i;
	}).then([]() {
		++i;
		return newPromise([](Defer d) {
			++i;
			d->reject(66);
		});
	}).fail([](int n) {
		++i;
	}).always([]() {
		++i;
		setTimeout([]() {
			testPerformance();
		}, 0);
	});
}

int main() {
    uv_loop_t *loop = uv_default_loop();

	testTimer();
	testPerformance();

    return uv_run(loop, UV_RUN_DEFAULT);
}
