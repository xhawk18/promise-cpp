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
#ifndef INC_PROMISE_HPP_
#define INC_PROMISE_HPP_

#include <exception>
#include <functional>
#include <memory>
#include <algorithm>
#include <typeinfo>
#include <list>

namespace promise {


template<class P, class M>
inline size_t pm_offsetof(const M P::*member) {
    return (size_t)&reinterpret_cast<char const&>((reinterpret_cast<P*>(0)->*member));
}

template<class P, class M>
inline P* pm_container_of(M* ptr, const M P::*member) {
    return reinterpret_cast<P*>(reinterpret_cast<char*>(ptr) - pm_offsetof(member));
}

//allocator
template <size_t SIZE>
struct memory_pool_buf {
    struct buf_t {
        buf_t(){}
        void *buf[(SIZE + sizeof(void *) - 1) / sizeof(void *)];
    };

    typename std::list<memory_pool_buf>::iterator self_;
    buf_t buf_;
};

template <size_t SIZE>
struct memory_pool {
    std::list<memory_pool_buf<SIZE> > used_;
    std::list<memory_pool_buf<SIZE> > free_;
};

template <size_t SIZE>
struct size_allocator {
    static inline memory_pool<SIZE> *get_memory_pool() {
        static memory_pool<SIZE> *pool_ = nullptr;
        if(pool_ == nullptr)
        pool_ = new memory_pool<SIZE>();
        return pool_;
    }
};

template <typename T>
struct allocator {
    enum allocator_type_t {
        kNew,
        kMalloc,
        kListPool
    };

    static const allocator_type_t allocator_type = kListPool;

    static inline void *obtain(size_t size) {
        if(allocator_type == kNew)
            return new typename memory_pool_buf<sizeof(T)>::buf_t;
        else if(allocator_type == kMalloc)
            return malloc(size);
        else if(allocator_type == kListPool){
            memory_pool<sizeof(T)> *pool = size_allocator<sizeof(T)>::get_memory_pool();
            if(pool->free_.size() > 0)
                pool->used_.splice(pool->used_.begin(), pool->free_, pool->free_.begin());
            else
                pool->used_.emplace_front();
            pool->used_.begin()->self_ = pool->used_.begin();
            return (void *)&pool->used_.begin()->buf_;
        }
        else return NULL;
    }
    
    static inline void release(void *ptr) {
        if(allocator_type == kNew){
            delete reinterpret_cast<typename memory_pool_buf<sizeof(T)>::buf_t *>(ptr);
            return;
        }
        else if(allocator_type == kMalloc){
            free(ptr);
            return;
        }
        else if(allocator_type == kListPool){
            memory_pool<sizeof(T)> *pool = size_allocator<sizeof(T)>::get_memory_pool();
            memory_pool_buf<sizeof(T)> *pool_buf = pm_container_of<memory_pool_buf<sizeof(T)>, typename memory_pool_buf<sizeof(T)>::buf_t>
                ((typename memory_pool_buf<sizeof(T)>::buf_t *)ptr, &memory_pool_buf<sizeof(T)>::buf_);
            pool->free_.splice(pool->free_.begin(), pool->used_, pool_buf->self_);
        }
    }
};


// Any library
// See http://www.boost.org/libs/any for Documentation.
// what:  variant type any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95
template<typename T>
struct remove_reference {
    typedef T type;
};
template<typename T>
struct remove_reference<T&> {
    typedef T type;
};
template<typename T>
struct remove_reference<const T&> {
    typedef T type;
};

template<class BaseT = void>
class any {
public: // structors
    any()
        : content(0) {
    }

    template<typename ValueType>
    any(const ValueType & value)
        : content(new holder<ValueType>(value)) {
    }

    any(const any & other)
        : content(other.content ? other.content->clone() : 0) {
    }

    ~any() {
        delete content;
    }

public: // modifiers

    any & swap(any & rhs) {
        std::swap(content, rhs.content);
        return *this;
    }

    template<typename ValueType>
    any & operator=(const ValueType & rhs) {
        any(rhs).swap(*this);
        return *this;
    }

    any & operator=(const any & rhs) {
        any(rhs).swap(*this);
        return *this;
    }

public: // queries
    bool empty() const {
        return !content;
    }
    
    void clear() {
        any().swap(*this);
    }

    const std::type_info & type() const {
        return content ? content->type() : typeid(void);
    }

public: // types (public so any_cast can be non-friend)
    class placeholder {
    public: // structors
        virtual ~placeholder() {
        }

    public: // queries
        virtual const std::type_info & type() const = 0;
        virtual placeholder * clone() const = 0;
        virtual BaseT *get_pointer() = 0;
    };

    template<typename ValueType>
    class holder : public placeholder {
    public: // structors
#if 1
        void* operator new(size_t size) {
            return allocator<holder>::obtain(size);
        }
        
        void operator delete(void *ptr) {
            allocator<holder>::release(ptr);
        }
#endif

        holder(const ValueType & value)
            : held(value) {
        }

    public: // queries
        virtual const std::type_info & type() const {
            return typeid(ValueType);
        }

        virtual placeholder * clone() const {
            return new holder(held);
        }

        inline virtual BaseT *get_pointer() {
            return static_cast<BaseT *>(&held);
        }

    public: // representation
        ValueType held;
    private: // intentionally left unimplemented
        holder & operator=(const holder &);
    };

public: // representation (public so any_cast can be non-friend)
    placeholder * content;
    BaseT *base() {
        return content->get_pointer();
    }
    const BaseT *base() const {
        return content->get_pointer();
    }
};

class bad_any_cast : public std::bad_cast {
public:
    virtual const char * what() const throw() {
        return "bad_any_cast: "
            "failed conversion using any_cast";
    }
};

template<typename ValueType, class BaseT>
ValueType * any_cast(any<BaseT> * operand) {
    typedef typename any<BaseT>::template holder<ValueType> holder_t;
    return operand &&
        operand->type() == typeid(ValueType)
        ? &static_cast<holder_t *>(operand->content)->held
        : 0;
}

template<typename ValueType, class BaseT>
inline const ValueType * any_cast(const any<BaseT> * operand) {
    return any_cast<ValueType>(const_cast<any<BaseT> *>(operand));
}

template<typename ValueType, class BaseT>
ValueType any_cast(any<BaseT> & operand) {
    typedef typename remove_reference<ValueType>::type nonref;

    nonref * result = any_cast<nonref>(&operand);
    if (!result)
        throw(bad_any_cast());
    return *result;
}

template<typename ValueType, class BaseT>
inline ValueType any_cast(const any<BaseT> & operand) {
    typedef typename remove_reference<ValueType>::type nonref;
    return any_cast<const nonref &>(const_cast<any<BaseT> &>(operand));
}

// Note: The "unsafe" versions of any_cast are not part of the
// public interface and may be removed at any time. They are
// required where we know what type is stored in the any and can't
// use typeid() comparison, e.g., when our types may travel across
// different shared libraries.
template<typename ValueType, class BaseT>
inline ValueType * unsafe_any_cast(any<BaseT> * operand) {
    typedef typename any<BaseT>::template holder<ValueType> holder_t;
    return &static_cast<holder_t *>(operand->content)->held;
}

template<typename ValueType, class BaseT>
inline const ValueType * unsafe_any_cast(const any<BaseT> * operand) {
    return unsafe_any_cast<ValueType>(const_cast<any<BaseT> *>(operand));
}
// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)




struct Promise;

template< class T >
class shared_ptr {
    typedef shared_ptr<Promise> Defer;
public:
    virtual ~shared_ptr() {
        dec_ref();
    }

    explicit shared_ptr(T *object)
        : object_(object) {
        add_ref();
    }
    
    explicit shared_ptr()
        : object_(nullptr) {
    }

    shared_ptr(shared_ptr const &ptr)
        : object_(ptr.object_) {
        add_ref();
    }

    shared_ptr &operator=(shared_ptr const &ptr) {
        shared_ptr(ptr).swap(*this);
        return *this;
    }

    bool operator==(shared_ptr const &ptr) const {
        return object_ == ptr.object_;
    }

    bool operator!=(shared_ptr const &ptr) const {
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
        shared_ptr().swap(*this);
    }

    void resolve() const {
        object_->resolve();
    }
    template <typename RET_ARG>
    void resolve(const RET_ARG &ret_arg) const {
        object_->resolve<RET_ARG>(ret_arg);
    }
    void reject() const {
        object_->reject();
    }
    template <typename RET_ARG>
    void reject(const RET_ARG &ret_arg) const {
        object_->reject<RET_ARG>(ret_arg);
    }
    template <typename FUNC_ON_RESOLVED, typename FUNC_ON_REJECTED>
    Defer then(FUNC_ON_RESOLVED on_resolved, FUNC_ON_REJECTED on_rejected) const {
        return object_->then<FUNC_ON_RESOLVED, FUNC_ON_REJECTED>(on_resolved, on_rejected);
    }

    template <typename FUNC_ON_RESOLVED>
    Defer then(FUNC_ON_RESOLVED on_resolved) const {
        return object_->then<FUNC_ON_RESOLVED>(on_resolved);
    }

    template <typename FUNC_ON_REJECTED>
    Defer fail(FUNC_ON_REJECTED on_rejected) const {
        return object_->fail<FUNC_ON_REJECTED>(on_rejected);
    }
    
    template <typename FUNC_ON_ALWAYS>
    Defer always(FUNC_ON_ALWAYS on_always) const {
        return object_->always<FUNC_ON_ALWAYS>(on_always);
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

    inline void swap(shared_ptr &ptr) {
        std::swap(object_, ptr.object_);
    }

    T *object_;
};

typedef shared_ptr<Promise> Defer;

typedef void(*FnSimple)();

struct Void {};

template <typename PROMISE_EX, typename RET, typename FUNC, typename RET_ARG>
struct ResolveChecker;
template <typename PROMISE_EX, typename RET, typename FUNC, typename RET_ARG>
struct RejectChecker;

template<typename FUNC>
struct GetRetArgType0 {
    typedef decltype(&FUNC::operator()) func_type;
    typedef typename GetRetArgType0<func_type>::ret_type ret_type;
    typedef typename GetRetArgType0<func_type>::arg_type arg_type;
};

template<typename RET, class T, class ARG>
struct GetRetArgType0< RET (T::*)(ARG) const > {
    typedef RET ret_type;
    typedef ARG arg_type;
};

template<typename RET, typename ARG>
struct GetRetArgType0< RET (*)(ARG) > {
    typedef RET ret_type;
    typedef ARG arg_type;
};

template<typename RET, typename T>
struct GetRetArgType0< RET (T::*)() const > {
    typedef RET ret_type;
    typedef Void arg_type;
};

template<typename RET>
struct GetRetArgType0< RET (*)() > {
    typedef RET ret_type;
    typedef Void arg_type;
};

template<typename RET>
struct CheckVoidType {
    typedef RET type;
    void operator()(const RET &) {
    }
};

template<>
struct CheckVoidType<void> {
    typedef Void type;
    void operator()() {
    }
};

template<typename RET>
struct GetRetArgType {
    typedef typename GetRetArgType0<RET>::ret_type ret_type0;
    typedef typename CheckVoidType<ret_type0>::type ret_type;
    typedef typename GetRetArgType0<RET>::arg_type arg_type;
};


template <typename Promise, typename FUNC_ON_RESOLVED, typename FUNC_ON_REJECTED>
struct PromiseEx 
    : public Promise {
    typedef typename GetRetArgType<FUNC_ON_RESOLVED>::ret_type resolve_ret_type;
    typedef typename GetRetArgType<FUNC_ON_RESOLVED>::arg_type resolve_arg_type;
    typedef typename GetRetArgType<FUNC_ON_REJECTED>::ret_type reject_ret_type;
    typedef typename GetRetArgType<FUNC_ON_REJECTED>::arg_type reject_arg_type;

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
    
    virtual Defer call_resolve(Defer &self, Promise *caller) {
        const FUNC_ON_RESOLVED &on_resolved = *reinterpret_cast<FUNC_ON_RESOLVED *>(Promise::on_resolved_);
        Defer d = ResolveChecker<PromiseEx, resolve_ret_type, FUNC_ON_RESOLVED, resolve_arg_type>::call(on_resolved, self, caller);
        clear_func();
        return d;
    }
    virtual Defer call_reject(Defer &self, Promise *caller) {
        const FUNC_ON_REJECTED &on_rejected = *reinterpret_cast<FUNC_ON_REJECTED *>(Promise::on_rejected_);
        Defer d = RejectChecker<PromiseEx, reject_ret_type, FUNC_ON_REJECTED, reject_arg_type>::call(on_rejected, self, caller);
        clear_func();
        return d;
    }

    void* operator new(size_t size) {
        return allocator<PromiseEx>::obtain(size);
    }
    
    void operator delete(void *ptr) {
        allocator<PromiseEx>::release(ptr);
    }
};

struct Promise {
    int ref_count_;
    Promise *prev_;
    Defer next_;
    any<> any_;
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
        any_.clear();
    }
    template <typename RET_ARG>
    void prepare_resolve(const RET_ARG &ret_arg) {
        if (status_ != kInit) return;
        status_ = kResolved;
        any_ = ret_arg;
    }

    void resolve() {
        prepare_resolve();
        if(status_ == kResolved)
            call_next();
    }
    template <typename RET_ARG>
    void resolve(const RET_ARG &ret_arg) {
        prepare_resolve(ret_arg);
        if(status_ == kResolved)
            call_next();
    }

    void prepare_reject() {
        if (status_ != kInit) return;
        status_ = kRejected;
        any_.clear();
    }
    template <typename RET_ARG>
    void prepare_reject(const RET_ARG &ret_arg) {
        if (status_ != kInit) return;
        status_ = kRejected;
        any_ = ret_arg;
    }
    void reject() {
        prepare_reject();
        if(status_ == kRejected)
            call_next();
    }
    template <typename RET_ARG>
    void reject(const RET_ARG &ret_arg) {
        prepare_reject(ret_arg);
        if(status_ == kRejected)
            call_next();
    }

    virtual Defer call_resolve(Defer &self, Promise *caller) = 0;
    virtual Defer call_reject(Defer &self, Promise *caller) = 0;

    template <typename FUNC>
    void run(FUNC func, Defer d) {
        try {
            func(d);
        } catch(const bad_any_cast &ex) {
            d->reject(ex);
        } catch(...) {
            d->reject(std::current_exception());
        }
    }
    
    Defer call_next() {
        if(status_ == kResolved) {
            if(next_.operator->()){
                status_ = kFinished;
                Defer d = next_->call_resolve(next_, this);
                if(d.operator->())
                    d->call_next();
                return d;
            }
        }
        else if(status_ == kRejected ) {
            if(next_.operator->()){
                status_ = kFinished;
                Defer d =  next_->call_reject(next_, this);
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

template <typename PROMISE_EX, typename RET, typename FUNC, typename RET_ARG>
struct ResolveChecker {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            self->prepare_resolve(func(any_cast<RET_ARG>(caller->any_)));
            caller->any_.clear();
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct ResolveChecker<PROMISE_EX, Void, FUNC, RET_ARG> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            func(any_cast<RET_ARG>(caller->any_));
            self->prepare_resolve();
            caller->any_.clear();
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX, typename RET, typename FUNC>
struct ResolveChecker<PROMISE_EX, RET, FUNC, Void> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            caller->any_.clear();
            self->prepare_resolve(func());
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC>
struct ResolveChecker<PROMISE_EX, Void, FUNC, Void> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            caller->any_.clear();
            func();
            self->prepare_resolve();
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct ResolveChecker<PROMISE_EX, Defer, FUNC, RET_ARG> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            Defer ret = func(any_cast<RET_ARG>(caller->any_));
            joinDeferObject(self, ret);
            caller->any_.clear();
            return ret;
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC>
struct ResolveChecker<PROMISE_EX, Defer, FUNC, Void> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            caller->any_.clear();
            Defer ret = func();
            joinDeferObject(self, ret);
            return ret;
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX>
struct ResolveChecker<PROMISE_EX, Void, FnSimple, Void> {
    static Defer call(const FnSimple &func, Defer &self, Promise *caller) {
        try {
            caller->any_.clear();
            if(func != nullptr)
                (*func)();
            self->prepare_resolve();
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};


template <typename PROMISE_EX, typename RET, typename FUNC, typename RET_ARG>
struct RejectChecker {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            if(caller->any_.type() == typeid(std::exception_ptr)){
                std::exception_ptr eptr = any_cast<std::exception_ptr>(caller->any_);
                caller->any_.clear();
                try{
                    std::rethrow_exception(eptr);
                }catch(const RET_ARG &ret_arg){
                    self->prepare_resolve(func(ret_arg));
                }
            }
            else {
                self->prepare_resolve(func(any_cast<RET_ARG>(caller->any_)));
                caller->any_.clear();
            }
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct RejectChecker<PROMISE_EX, Void, FUNC, RET_ARG> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            if(caller->any_.type() == typeid(std::exception_ptr)){
                std::exception_ptr eptr = any_cast<std::exception_ptr>(caller->any_);
                caller->any_.clear();
                try{
                    std::rethrow_exception(eptr);
                }catch(const RET_ARG &ret_arg){
                    func(ret_arg);
                    self->prepare_resolve();
                }
            }
            else{
                func(any_cast<RET_ARG>(caller->any_));
                self->prepare_resolve();
                caller->any_.clear();
            }
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX, typename RET, typename FUNC>
struct RejectChecker<PROMISE_EX, RET, FUNC, Void> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            caller->any_.clear();
            self->prepare_resolve(func());
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC>
struct RejectChecker<PROMISE_EX, Void, FUNC, Void> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            caller->any_.clear();
            func();
            self->prepare_resolve();
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};


template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct RejectChecker<PROMISE_EX, Defer, FUNC, RET_ARG> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            if(caller->any_.type() == typeid(std::exception_ptr)){
                std::exception_ptr eptr = any_cast<std::exception_ptr>(caller->any_);
                caller->any_.clear();
                try{
                    std::rethrow_exception(eptr);
                }catch(const RET_ARG &ret_arg){
                    Defer ret = func(ret_arg);
                    joinDeferObject(self, ret);
                    return ret;
                }
            }
            else{
                Defer ret = func(any_cast<RET_ARG>(caller->any_));
                joinDeferObject(self, ret);
                caller->any_.clear();
                return ret;
            }
        }
        catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        }
        catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC>
struct RejectChecker<PROMISE_EX, Defer, FUNC, Void> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
        try {
            caller->any_.clear();
            Defer ret = func();
            joinDeferObject(self, ret);
            return ret;
        }
        catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
        }
        catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
        }
        return self;
    }
};

template <typename PROMISE_EX>
struct RejectChecker<PROMISE_EX, Void, FnSimple, Void> {
    static Defer call(const FnSimple &func, Defer &self, Promise *caller) {
        try {
            caller->any_.clear();
            if (func != nullptr) {
                (*func)();
                self->prepare_resolve();
                return self;
            }
        } catch(const bad_any_cast &) {
            self->prepare_reject(caller->any_);
            caller->any_.clear();
            return self;
        } catch(...) {
            caller->any_.clear();
            self->prepare_reject(std::current_exception());
            return self;
        }

        self->any_.swap(caller->any_);
        self->prepare_reject();
        return self;
    }
};


template <typename FUNC>
Defer newPromise(FUNC func) {
    /* Here func in PromiseEx will never be called.
       To save func in PromiseEx is just to keep reference of the object */
    Defer promise(new PromiseEx<Promise, FnSimple, FnSimple>(nullptr, nullptr));
    promise->run(func, promise);
    return promise;
}


}
#endif
