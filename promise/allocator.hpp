#pragma once
#ifndef INC_PM_ALLOCATOR_HPP_
#define INC_PM_ALLOCATOR_HPP_

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

#ifdef PM_MULTITHREAD
#include <mutex>
#endif

#include <memory>

namespace promise {

inline void destroy_node(pm_list *node);

struct pm_memory_pool {
    pm_list free_;
    size_t size_;
    pm_memory_pool(size_t size)
        : free_()
        , size_(size){
    }
    ~pm_memory_pool(){
        pm_list *node = free_.next();
        while(node != &free_){
            pm_list *next = node->next();
            destroy_node(node);
            node = next;
        }
    }
};


//allocator
struct pm_memory_pool_buf_header {
    pm_memory_pool_buf_header(pm_memory_pool *pool)
        : pool_(pm_stack::ptr_to_itr(reinterpret_cast<void *>(pool)))
        , ref_count_(0){
    }

    pm_list list_;
    pm_stack::itr_t pool_;
#ifndef PM_EMBED
    size_t ref_count_;
#else
    uint16_t ref_count_;
#endif
};


struct dummy_pool_buf {
    pm_memory_pool_buf_header header_;
    struct {
        void *buf_[1];
    } buf_;

    static inline void *to_ptr(pm_memory_pool_buf_header *header) {
        dummy_pool_buf *buf = reinterpret_cast<dummy_pool_buf *>(
            reinterpret_cast<char *>(header) - pm_offsetof(&dummy_pool_buf::header_));
        return (void *)&buf->buf_;
    }

    static inline void *to_ptr(pm_list *list) {
        pm_memory_pool_buf_header *header = pm_container_of(list, &pm_memory_pool_buf_header::list_);
        return dummy_pool_buf::to_ptr(header);
    }

    static inline pm_memory_pool_buf_header *from_ptr(void *ptr) {
        dummy_pool_buf *buf = pm_container_of(ptr, &dummy_pool_buf::buf_);
        return &buf->header_;
    }
};

inline void destroy_node(pm_list *node) {
    pm_memory_pool_buf_header *header = pm_container_of(node, &pm_memory_pool_buf_header::list_);
    header->~pm_memory_pool_buf_header();
#ifdef PM_EMBED_STACK
    //nothing to be freed
#else
    dummy_pool_buf *pool_buf = pm_container_of
        (header, &dummy_pool_buf::header_);
    void **buf = reinterpret_cast<void **>(pool_buf);
    delete[] buf;
#endif
}

template <size_t SIZE>
struct pm_size_allocator {
    static inline pm_memory_pool *get_memory_pool() {
        thread_local static std::unique_ptr<pm_memory_pool> pool_;
        if(pool_ == nullptr)
            pool_.reset(pm_stack_new<pm_memory_pool>(SIZE));
        return pool_.get();
    }
};

struct pm_allocator {
private:
    static pm_memory_pool_buf_header *obtain_pool_buf(pm_memory_pool *pool){
        pm_list *node = pool->free_.next();
        node->detach();
        pm_memory_pool_buf_header *header = pm_container_of(node, &pm_memory_pool_buf_header::list_);
        return header;
    }

    static void *obtain_impl(pm_memory_pool *pool, size_t size) {
#ifdef PM_DEBUG
        (*dbg_alloc_size()) += (uint32_t)size;
#endif
        if (pool->free_.empty()) {
            size_t alloc_size = size + pm_offsetof(&dummy_pool_buf::buf_);
#ifdef PM_EMBED_STACK
            void *buf = pm_stack::allocate(alloc_size);
#else
            alloc_size = (alloc_size + sizeof(void *) - 1) / sizeof(void *);
            void **buf = new void *[alloc_size];
            //printf("new %p\n", buf);
#endif
            pm_memory_pool_buf_header *header = new(buf)
                pm_memory_pool_buf_header(pool);
            dummy_pool_buf *pool_buf = pm_container_of
                (header, &dummy_pool_buf::header_);
            return (void *)&pool_buf->buf_;
        }
        else {
            pm_memory_pool_buf_header *header = obtain_pool_buf(pool);
            dummy_pool_buf *pool_buf = pm_container_of
                (header, &dummy_pool_buf::header_);
            //printf("++++ obtain = %p %d\n", (void *)&pool_buf->buf_, sizeof(T));
            return (void *)&pool_buf->buf_;
        }
    }

    static void release(void *ptr) {
        //printf("--- release = %p\n", ptr);
        dummy_pool_buf *pool_buf = pm_container_of(ptr, &dummy_pool_buf::buf_);
        pm_memory_pool_buf_header *header = &pool_buf->header_;
        pm_memory_pool *pool = reinterpret_cast<pm_memory_pool *>(pm_stack::itr_to_ptr(header->pool_));

#if defined PM_EMBED_STACK || !defined PM_NO_ALLOC_CACHE
        pool->free_.move(&header->list_);
#else
        header->~pm_memory_pool_buf_header();
        void **buf = reinterpret_cast<void **>(pool_buf);
        //printf("delete %p\n", buf);
        delete[] buf;
#endif

#ifdef PM_DEBUG
        (*dbg_alloc_size()) -= (uint32_t)pool->size_;
#endif
    }

    static void add_ref_impl(void *object) {
        //printf("add_ref %p\n", object);
        if (object != nullptr) {
            pm_memory_pool_buf_header *header = dummy_pool_buf::from_ptr(object);
            //printf("++ %p %d -> %d\n", pool_buf, pool_buf->ref_count_, pool_buf->ref_count_ + 1);

#ifdef PM_MULTITHREAD
            std::lock_guard<std::recursive_mutex> lock(pm_mutex::get_mutex());
#endif
            ++header->ref_count_;

            //Check if ref_count_ must overflow£¡
            if (header->ref_count_ <= 0) {
                pm_throw("ref_count_ overflow");
            }
        }
    }

    static bool dec_ref_impl(void *object) {
        //printf("dec_ref %p\n", object);
        if (object != nullptr) {
            pm_memory_pool_buf_header *header = dummy_pool_buf::from_ptr(object);
            //printf("-- %p %d -> %d\n", pool_buf, pool_buf->ref_count_, pool_buf->ref_count_ - 1);

#ifdef PM_MULTITHREAD
            std::lock_guard<std::recursive_mutex> lock(pm_mutex::get_mutex());
#endif
            pm_assert(header->ref_count_ > 0);
            --header->ref_count_;
            if (header->ref_count_ == 0) {
                return true;
            }
        }
        return false;
    }

public:
    template <typename T>
    static inline void *obtain() {
        pm_memory_pool *pool = pm_size_allocator<sizeof(T)>::get_memory_pool();
        return obtain_impl(pool, sizeof(T));
    }

    template<typename T>
    static void add_ref(T *object) {
        add_ref_impl(reinterpret_cast<void *>(const_cast<T *>(object)));
    }

    template<typename T>
    static void dec_ref(T *object) {
        void *object_ = reinterpret_cast<void *>(const_cast<T *>(object));
        if(dec_ref_impl(object_)){
            object->~T();
            pm_allocator::release(object_);
        }
    }
};

template< class T, class... Args >
inline T *pm_new(Args&&... args) {
    T *object = new(pm_allocator::template obtain<T>()) T(args...);
    pm_allocator::add_ref(object);
    return object;
}

template< class T >
inline void pm_delete(T *object){
    pm_allocator::dec_ref(object);
}

}
#endif
