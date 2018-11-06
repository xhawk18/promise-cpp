#pragma once
#ifndef INC_PM_STACK_HPP_
#define INC_PM_STACK_HPP_

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

namespace promise {

#ifdef PM_EMBED_STACK
inline constexpr size_t pm_log(size_t n) {
    return (n <= 1 ? 0 : 1 + pm_log(n >> 1));
}

template<bool match_uint8, bool match_uint16, bool match_uint32>
struct pm_offset_impl {
    typedef uint64_t type;
};
template<bool match_uint16, bool match_uint32>
struct pm_offset_impl<true, match_uint16, match_uint32> {
    typedef uint8_t type;
};
template<bool match_uint32>
struct pm_offset_impl<false, true, match_uint32> {
    typedef uint16_t type;
};
template<>
struct pm_offset_impl<false, false, true> {
    typedef uint32_t type;
};



template<size_t SIZE, size_t ADDR_ALIGN>
struct pm_offset {
    typedef typename pm_offset_impl<
        ((SIZE + ADDR_ALIGN - 1) / ADDR_ALIGN <= 0x100),
        ((SIZE + ADDR_ALIGN - 1) / ADDR_ALIGN <= 0x10000UL),
        ((SIZE + ADDR_ALIGN - 1) / ADDR_ALIGN <= 0x100000000ULL) >::type type;
};
#endif

//allocator
struct pm_stack {
#ifdef PM_EMBED_STACK
    static inline char *start() {
        static void *buf_[(PM_EMBED_STACK + sizeof(void *) - 1) / sizeof(void *)];
        return (char *)buf_;
    }

    static inline void *allocate(size_t size) {
        static char *top = start();
        char *start_ = start();

        size = (size + sizeof(void *) - 1) / sizeof(void *) * sizeof(void *);
        if (start_ + PM_EMBED_STACK < top + size)
            pm_throw("no_mem");

        void *ret = top;
        top += size;

#ifdef PM_DEBUG
        (*dbg_stack_size()) = (uint32_t)(top - start_);
#endif
        //printf("mem ======= %d %d, size = %d, %d, %d, %x\n", (int)(top - start_), (int)sizeof(void *), (int)size, (int)OFFSET_IGNORE_BIT, (int)sizeof(itr_t), (int)ret);
        return ret;
    }

    static const size_t OFFSET_IGNORE_BIT = pm_log(sizeof(void *));
    typedef pm_offset<PM_EMBED_STACK, sizeof(void *)>::type itr_t;
    //static const size_t OFFSET_IGNORE_BIT = 0;
    //typedef pm_offset<PM_EMBED_STACK, 1>::type itr_t;

    static inline itr_t ptr_to_itr(void *ptr) {
        if(ptr == nullptr) return (itr_t)-1;
        else return (itr_t)(((char *)ptr - (char *)pm_stack::start()) >> OFFSET_IGNORE_BIT);
    }

    static inline void *itr_to_ptr(itr_t itr) {
        if(itr == (itr_t)-1) return nullptr;
        else return (void *)((char *)pm_stack::start() + ((ptrdiff_t)itr << OFFSET_IGNORE_BIT));
    }
#else
    static const size_t OFFSET_IGNORE_BIT = 0;
    typedef void *itr_t;
    static inline itr_t ptr_to_itr(void *ptr) {
        return ptr;
    }

    static inline void *itr_to_ptr(itr_t itr) {
        return itr;
    }
#endif
};

template< class T, class... Args >
inline T *pm_stack_new(Args&&... args) {
    return new
#ifdef PM_EMBED_STACK
        (pm_stack::allocate(sizeof(T)))
#endif
        T(args...);
}

}
#endif
