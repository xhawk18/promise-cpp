#pragma once
#ifndef INC_PROMISE_MIN_HPP_
#define INC_PROMISE_MIN_HPP_

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

#define PM_EMBED_STACK 4096

#include <memory>
#include <typeinfo>
#include <stdint.h>

namespace promise {


template<class P, class M>
inline size_t pm_offsetof(const M P::*member) {
    return (size_t)&reinterpret_cast<char const&>((reinterpret_cast<P*>(0)->*member));
}

template<class P, class M>
inline P* pm_container_of(M* ptr, const M P::*member) {
    return reinterpret_cast<P*>(reinterpret_cast<char*>(ptr) - pm_offsetof(member));
}

template<typename T>
inline void pm_throw(const T &t){
    while(1);
}

//allocator
#ifdef PM_EMBED_STACK
struct pm_stack {
    static inline void *start() {
        static void *buf_[(PM_EMBED_STACK + sizeof(void *) - 1) / sizeof(void *)];
        return buf_;
    }

    static inline void *allocate(size_t size) {
        static char *top_ = (char *)start();
        void *start_ = start();
        size = (size + sizeof(void *) - 1) / sizeof(void *) * sizeof(void *);
        if ((char *)start_ + PM_EMBED_STACK < top_ + size)
            pm_throw("no_mem");

        void *ret = (void *)top_;
        top_ += size;
        //printf("mem ======= %d %d\n", top_ - (char *)start_, sizeof(void *));
        return ret;
    }
};
#endif

//List
struct pm_list {
#ifdef PM_EMBED_STACK
    typedef uint16_t ptr_t;
    static const int PTR_IGNORE_BIT = 2;
    
    inline ptr_t list_to_ptr(pm_list *list) {
        return (ptr_t)(((char *)list - (char *)pm_stack::start()) >> PTR_IGNORE_BIT);
    }

    inline pm_list *ptr_to_list(ptr_t ptr) {
        return (pm_list *)((char *)pm_stack::start() + ((ptrdiff_t)ptr << PTR_IGNORE_BIT));
    }
#else
    typedef pm_list *ptr_t;

    inline ptr_t list_to_ptr(pm_list *list) {
        return list;
    }

    inline pm_list *ptr_to_list(ptr_t ptr) {
        return ptr;
    }

#endif
    ptr_t prev_;
    ptr_t next_;

    pm_list()
        : prev_(list_to_ptr(this))
        , next_(list_to_ptr(this)) {
    }

    inline pm_list *prev() {
        return ptr_to_list(prev_);
    }

    inline pm_list *next() {
        return ptr_to_list(next_);
    }

    inline void prev(pm_list *other) {
        prev_ = list_to_ptr(other);
    }

    inline void next(pm_list *other) {
        next_ = list_to_ptr(other);
    }

    /* Connect or disconnect two lists. */
    static void toggleConnect(pm_list *list1, pm_list *list2) {
        pm_list *prev1 = list1->prev();
        pm_list *prev2 = list2->prev();
        prev1->next(list2);
        prev2->next(list1);
        list1->prev(prev2);
        list2->prev(prev1);
    }

    /* Connect two lists. */
    static void connect(pm_list *list1, pm_list *list2) {
        toggleConnect(list1, list2);
    }

    /* Disconnect tow lists. */
    static void disconnect(pm_list *list1, pm_list *list2) {
        toggleConnect(list1, list2);
    }

    /* Same as listConnect */
    void attach(pm_list *node) {
        connect(this, node);
    }

    /* Make node in detach mode */
    void detach () {
        disconnect(this, this->next());
    }

    /* Move node to list, after moving,
       node->next == this
       this->prev == node
     */
    void move(pm_list *node) {
#if 1
        node->prev()->next(node->next());
        node->next()->prev(node->prev());

        node->next(this);
        node->prev(this->prev());
        this->prev()->next(node);
        this->prev(node);
#else
        detach(node);
        attach(this, node);
#endif
    }

    /* Check if list is empty */
    int empty() {
        return (this->next() == this);
    }
};


//allocator
template <size_t SIZE>
struct pm_memory_pool_buf {
    struct buf_t {
        buf_t(){}
        void *buf[(SIZE + sizeof(void *) - 1) / sizeof(void *)];
    };

    pm_memory_pool_buf() {
    }
    buf_t buf_;
    pm_list list_;
};

template <size_t SIZE>
struct pm_memory_pool {
    pm_list used_;
    pm_list free_;
    pm_memory_pool(){
    }
};

template <size_t SIZE>
struct pm_size_allocator {
    static inline pm_memory_pool<SIZE> *get_memory_pool() {
        static pm_memory_pool<SIZE> *pool_ = nullptr;
        if(pool_ == nullptr)
            pool_ = new
#ifdef PM_EMBED_STACK
                (pm_stack::allocate(sizeof(*pool_)))
#endif
                pm_memory_pool<SIZE>();
        return pool_;
    }
};

template <typename T>
struct pm_allocator {
    static inline void *obtain(size_t size) {
        pm_memory_pool<sizeof(T)> *pool = pm_size_allocator<sizeof(T)>::get_memory_pool();
        if (pool->free_.empty()) {
            pm_memory_pool_buf<sizeof(T)> *pool_buf = new
#ifdef PM_EMBED_STACK
                (pm_stack::allocate(sizeof(*pool_buf)))
#endif
                pm_memory_pool_buf<sizeof(T)>;
            pool->used_.attach(&pool_buf->list_);
            return (void *)&pool_buf->buf_;
        }
        else {
            pm_list *node = pool->free_.next();
            pool->used_.move(node);
            pm_memory_pool_buf<sizeof(T)> *pool_buf = pm_container_of<pm_memory_pool_buf<sizeof(T)>, pm_list>
                (node, &pm_memory_pool_buf<sizeof(T)>::list_);
            return (void *)&pool_buf->buf_;
        }
    }
    
    static inline void release(void *ptr) {
        pm_memory_pool<sizeof(T)> *pool = pm_size_allocator<sizeof(T)>::get_memory_pool();
        pm_memory_pool_buf<sizeof(T)> *pool_buf = pm_container_of<pm_memory_pool_buf<sizeof(T)>, typename pm_memory_pool_buf<sizeof(T)>::buf_t>
            ((typename pm_memory_pool_buf<sizeof(T)>::buf_t *)ptr, &pm_memory_pool_buf<sizeof(T)>::buf_);
        pool->free_.move(&pool_buf->list_);
    }
};


template<typename FUNC>
struct func_traits {
	typedef decltype((*(FUNC*)0)()) ret_type;
};


struct Promise;

template< class T >
class pm_shared_ptr {
    typedef pm_shared_ptr<Promise> Defer;
public:
    virtual ~pm_shared_ptr() {
        dec_ref();
    }

    explicit pm_shared_ptr(T *object)
        : object_(object) {
        add_ref();
    }
    
    explicit pm_shared_ptr()
        : object_(nullptr) {
    }

    pm_shared_ptr(pm_shared_ptr const &ptr)
        : object_(ptr.object_) {
        add_ref();
    }

    pm_shared_ptr &operator=(pm_shared_ptr const &ptr) {
        pm_shared_ptr(ptr).swap(*this);
        return *this;
    }

    bool operator==(pm_shared_ptr const &ptr) const {
        return object_ == ptr.object_;
    }

    bool operator!=(pm_shared_ptr const &ptr) const {
        return !( *this == ptr );
    }

    bool operator==(T const *ptr) const {
        return object_ == ptr;
    }

    bool operator!=(T const *ptr) const {
        return !( *this == ptr );
    }

    inline T *operator->() const {
        return object_;
    }
    
    inline T *obtain_rawptr(){
        add_ref();
        return object_;
    }
    
    inline void release_rawptr(){
        dec_ref();
    }

    Defer find_pending() const {
        return object_->find_pending();
    }
    
    void clear() {
        pm_shared_ptr().swap(*this);
    }

    void resolve() const {
        object_->resolve();
    }

    void reject() const {
        object_->reject();
    }

    template <typename FUNC_ON_RESOLVED, typename FUNC_ON_REJECTED>
    Defer then(FUNC_ON_RESOLVED on_resolved, FUNC_ON_REJECTED on_rejected) const {
        return object_->template then<FUNC_ON_RESOLVED, FUNC_ON_REJECTED>(on_resolved, on_rejected);
    }

    template <typename FUNC_ON_RESOLVED>
    Defer then(FUNC_ON_RESOLVED on_resolved) const {
        return object_->template then<FUNC_ON_RESOLVED>(on_resolved);
    }

    template <typename FUNC_ON_REJECTED>
    Defer fail(FUNC_ON_REJECTED on_rejected) const {
        return object_->template fail<FUNC_ON_REJECTED>(on_rejected);
    }
    
    template <typename FUNC_ON_ALWAYS>
    Defer always(FUNC_ON_ALWAYS on_always) const {
        return object_->template always<FUNC_ON_ALWAYS>(on_always);
    }   
private:
    void add_ref() {
        if (object_ != nullptr) {
            //printf("++ %p %d -> %d\n", object_, object_->ref_count_, object_->ref_count_ + 1);
            ++object_->ref_count_;
        }
    }
    
    void dec_ref() {
        if(object_ != nullptr) {
            //printf("-- %p %d -> %d\n", object_, object_->ref_count_, object_->ref_count_ - 1);
            --object_->ref_count_;
            if(object_->ref_count_ == 0)
                delete object_;
        }
    }

    inline void swap(pm_shared_ptr &ptr) {
        std::swap(object_, ptr.object_);
    }

    T *object_;
};

typedef pm_shared_ptr<Promise> Defer;

typedef void(*FnSimple)();

template <typename RET, typename FUNC>
struct ResolveChecker;
template <typename RET, typename FUNC>
struct RejectChecker;

template <typename Promise, typename FUNC_ON_RESOLVED, typename FUNC_ON_REJECTED>
struct PromiseEx 
    : public Promise {
    typedef typename func_traits<FUNC_ON_RESOLVED>::ret_type resolve_ret_type;
    typedef typename func_traits<FUNC_ON_REJECTED>::ret_type reject_ret_type;

    struct {
        void *buf[(sizeof(FUNC_ON_RESOLVED) + sizeof(void *) - 1)/ sizeof(void *)];
    } func_buf0_t;
    struct {
        void *buf[(sizeof(FUNC_ON_REJECTED) + sizeof(void *) - 1) / sizeof(void *)];
    } func_buf1_t;

    PromiseEx(const FUNC_ON_RESOLVED &on_resolved, const FUNC_ON_REJECTED &on_rejected)
        : Promise(reinterpret_cast<void *>(new(&func_buf0_t) FUNC_ON_RESOLVED(on_resolved)),
            reinterpret_cast<void *>(new(&func_buf1_t) FUNC_ON_REJECTED(on_rejected))) {
    }

    virtual ~PromiseEx() {
        clear_func();
    }
    
    void clear_func() {
        if(Promise::on_resolved_ != nullptr) {
            reinterpret_cast<FUNC_ON_RESOLVED *>(Promise::on_resolved_)->~FUNC_ON_RESOLVED();
            Promise::on_resolved_ = nullptr;
        }
        if(Promise::on_rejected_ != nullptr) {
            reinterpret_cast<FUNC_ON_REJECTED *>(Promise::on_rejected_)->~FUNC_ON_REJECTED();
            Promise::on_rejected_ = nullptr;
        }
    }
    
    virtual Defer call_resolve(Defer &self) {
        const FUNC_ON_RESOLVED &on_resolved = *reinterpret_cast<FUNC_ON_RESOLVED *>(Promise::on_resolved_);
        Defer d = ResolveChecker<resolve_ret_type, FUNC_ON_RESOLVED>::call(on_resolved, self);
        clear_func();
        return d;
    }
    virtual Defer call_reject(Defer &self) {
        const FUNC_ON_REJECTED &on_rejected = *reinterpret_cast<FUNC_ON_REJECTED *>(Promise::on_rejected_);
        Defer d = RejectChecker<reject_ret_type, FUNC_ON_REJECTED>::call(on_rejected, self);
        clear_func();
        return d;
    }

    void* operator new(size_t size) {
        return pm_allocator<PromiseEx>::obtain(size);
    }
    
    void operator delete(void *ptr) {
        pm_allocator<PromiseEx>::release(ptr);
    }
};

struct Promise {
    int ref_count_;
    Promise *prev_;
    Defer next_;
    void *on_resolved_;
    void *on_rejected_;
    
    enum status_t {
        kInit,
        kResolved,
        kRejected,
        kFinished
    };
    status_t status_;
    
    Promise(const Promise &) = delete;
    explicit Promise(void *on_resolved, void *on_rejected)
        : ref_count_(0)
        , prev_(nullptr)
        , next_(nullptr)
        , on_resolved_(on_resolved)
        , on_rejected_(on_rejected)
        , status_(kInit) {
    }
    
    virtual ~Promise() {
        if (next_.operator->()) {
            next_->prev_ = nullptr;
        }
    }
    
    void prepare_resolve() {
        if (status_ != kInit) return;
        status_ = kResolved;
    }

    void resolve() {
        prepare_resolve();
        if(status_ == kResolved)
            call_next();
    }

    void prepare_reject() {
        if (status_ != kInit) return;
        status_ = kRejected;
    }

    void reject() {
        prepare_reject();
        if(status_ == kRejected)
            call_next();
    }

    virtual Defer call_resolve(Defer &self) = 0;
    virtual Defer call_reject(Defer &self) = 0;

    template <typename FUNC>
    void run(FUNC func, Defer d) {
        func(d);
    }
    
    Defer call_next() {
        if(status_ == kResolved) {
            if(next_.operator->()){
                status_ = kFinished;
                Defer d = next_->call_resolve(next_);
                if(d.operator->())
                    d->call_next();
                return d;
            }
        }
        else if(status_ == kRejected ) {
            if(next_.operator->()){
                status_ = kFinished;
                Defer d =  next_->call_reject(next_);
                if (d.operator->())
                    d->call_next();
                return d;
            }
        }

        return next_;
    }

    template <typename FUNC_ON_RESOLVED, typename FUNC_ON_REJECTED>
    Defer then(FUNC_ON_RESOLVED on_resolved, FUNC_ON_REJECTED on_rejected) {
        Defer promise(new PromiseEx<Promise, FUNC_ON_RESOLVED, FUNC_ON_REJECTED>(on_resolved, on_rejected));
        next_ = promise;
        promise->prev_ = this;
        return call_next();
    }

    template <typename FUNC_ON_RESOLVED>
    Defer then(FUNC_ON_RESOLVED on_resolved) {
        return then<FUNC_ON_RESOLVED, FnSimple>(on_resolved, nullptr);
    }

    template <typename FUNC_ON_REJECTED>
    Defer fail(FUNC_ON_REJECTED on_rejected) {
        return then<FnSimple, FUNC_ON_REJECTED>(nullptr, on_rejected);
    }
    
    template <typename FUNC_ON_ALWAYS>
    Defer always(FUNC_ON_ALWAYS on_always) {
        return then<FUNC_ON_ALWAYS, FUNC_ON_ALWAYS>(on_always, on_always);
    }

    Defer find_pending() {
        if (status_ == kInit) {
            Promise *p = this;
            Promise *prev = p->prev_;
            while (prev != nullptr) {
                if (prev->status_ != kInit)
                    return prev->next_;
                p = prev;
                prev = p->prev_;
            }
            return Defer(p);
        }
        else {
            Promise *p = this;
            Promise *next = p->next_.operator->();
            while (next != nullptr) {
                if (next->status_ == kInit)
                    return p->next_;
                p = next;
                next = p->next_.operator->();
            }
            return Defer();
        }
    }
};

inline void joinDeferObject(Defer &self, Defer &next){
    if(self->next_.operator->())
        self->next_->prev_ = next.operator->();
    next->next_ = self->next_;
    self->next_ = next;
    next->prev_ = self.operator->();
}

template <typename RET, typename FUNC>
struct ResolveChecker {
    static Defer call(const FUNC &func, Defer &self) {
		func();
        self->prepare_resolve();
        return self;
    }
};

template <typename FUNC>
struct ResolveChecker<Defer, FUNC> {
    static Defer call(const FUNC &func, Defer &self) {
		Defer ret = func();
        joinDeferObject(self, ret);
        return ret;
    }
};


template <typename RET>
struct ResolveChecker<RET, FnSimple> {
    static Defer call(const FnSimple &func, Defer &self) {
        if (func != nullptr)
			func();
        self->prepare_resolve();
        return self;
    }
};


template <typename RET, typename FUNC>
struct RejectChecker {
    static Defer call(const FUNC &func, Defer &self) {
		func();
        self->prepare_resolve();
        return self;
    }
};

template <typename FUNC>
struct RejectChecker<Defer, FUNC> {
    static Defer call(const FUNC &func, Defer &self) {
        Defer ret = func();
        joinDeferObject(self, ret);
        return ret;
    }
};

template <typename RET>
struct RejectChecker<RET, FnSimple> {
    static Defer call(const FnSimple &func, Defer &self) {
        if (func != nullptr) {
			func();
            self->prepare_resolve();
            return self;
        }
        self->prepare_reject();
        return self;
    }
};

/* Create new promise object */
template <typename FUNC>
Defer newPromise(FUNC func) {
    Defer promise(new PromiseEx<Promise, FnSimple, FnSimple>(nullptr, nullptr));
    promise->run(func, promise);
    return promise;
}

/* Loop while func call resolved */
template <typename FUNC>
Defer While(FUNC func) {
    return newPromise(func).then([func]() {
        return While(func);
    });
}


}
#endif
