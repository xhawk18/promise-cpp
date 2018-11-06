#pragma once
#ifndef INC_PM_DEBUG_HPP_
#define INC_PM_DEBUG_HPP_

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

#ifdef PM_DEBUG
#include <cassert>
#define PM_TYPE_NONE    0
//#define PM_TYPE_TIMER   1

#define PM_MAX_CALL_LEN 50
#define pm_assert(x)    assert(x)

extern "C" {
    inline uint32_t *dbg_alloc_size() {
        static uint32_t alloc_size = 0;
        return &alloc_size;
    }

    inline uint32_t *dbg_stack_size() {
        static uint32_t stack_size = 0;
        return &stack_size;
    }

    inline uint32_t *dbg_promise_call_len() {
        static uint32_t promise_call_len = 0;
        return &promise_call_len;
    };

    inline uint32_t *dbg_promise_id() {
        static uint32_t promise_id = 0;
        return &promise_id;
    };
}
#else
#define pm_assert(x)    do{ } while(0)
#endif

#endif
