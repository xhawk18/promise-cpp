/*
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

#pragma once
#ifndef INC_PROMISE_FULL_HPP_
#define INC_PROMISE_FULL_HPP_

//
// C++ promise/A+ library in Javascript styles.
// See Readme.md for functions and detailed usage --
//

// support multithread
// #define PM_MULTITHREAD

// macro for debug
// #define PM_DEBUG

// enable PM_EMBED to remove some features so as to run on embedded CPU
// #define PM_EMBED

// define PM_EMBED_STACK to use a pre-defined static buffer for memory allocation
// #define PM_EMBED_STACK (512*1024*1024)

// define PM_NO_ALLOC_CACHE to disable memory cache in this library.
// #define PM_NO_ALLOC_CACHE

#include <cstdio>
#include <memory>
#include <typeinfo>
#include <utility>
#include <algorithm>
#include <iterator>
#include <functional>
#ifdef PM_MULTITHREAD
#include <mutex>
#endif

#ifndef PM_EMBED
#include <exception>
#include <vector>
#endif

#include "promise/std_addon.hpp"
#include "promise/misc.hpp"
#include "promise/debug.hpp"
#include "promise/stack.hpp"
#include "promise/list.hpp"
#include "promise/allocator.hpp"
#include "promise/any.hpp"

namespace promise {


template<typename T>
struct tuple_remove_reference {
    typedef typename std::remove_reference<T>::type r_type;
    typedef typename std::remove_cv<r_type>::type type;
};

template<typename T, std::size_t SIZE>
struct tuple_remove_reference<T[SIZE]> {
    typedef typename tuple_remove_reference<const T *>::type type;
};

template<typename TUPLE>
struct remove_reference_tuple {
    static const std::size_t size_ = std::tuple_size<TUPLE>::value;

    template<size_t SIZE, std::size_t... I>
    struct converted {
        typedef std::tuple<typename tuple_remove_reference<typename std::tuple_element<I, TUPLE>::type>::type...> type;
    };

    template<std::size_t... I>
    static converted<size_, I...> get_type(const std::index_sequence<I...> &) {
        return converted<size_, I...>();
    }

    typedef decltype(get_type(std::make_index_sequence<size_>())) converted_type;
    typedef typename converted_type::type type;
};

template<typename ...ARG>
struct arg_traits {
    typedef std::tuple<ARG...> type2;
    typedef typename remove_reference_tuple<type2>::type type;
    enum { direct_any = false };
};

template<>
struct arg_traits<pm_any &> {
    typedef pm_any type;
    enum { direct_any = true };
};

template<typename FUNC>
struct func_traits_impl {
    typedef decltype(&FUNC::operator()) func_type;
    typedef typename func_traits_impl<func_type>::ret_type ret_type;
    typedef typename func_traits_impl<func_type>::arg_type arg_type;
    enum { direct_any = func_traits_impl<func_type>::direct_any };
};

template<typename RET, class T, typename ...ARG>
struct func_traits_impl< RET(T::*)(ARG...) const > {
    typedef RET ret_type;
    typedef typename arg_traits<ARG...>::type arg_type;
    enum { direct_any = arg_traits<ARG...>::direct_any };
};

template<typename RET, typename ...ARG>
struct func_traits_impl< RET(*)(ARG...) > {
    typedef RET ret_type;
    typedef typename arg_traits<ARG...>::type arg_type;
    enum { direct_any = arg_traits<ARG...>::direct_any };
};

template<typename RET, typename ...ARG>
struct func_traits_impl< RET(ARG...) > {
    typedef RET ret_type;
    typedef typename arg_traits<ARG...>::type arg_type;
    enum { direct_any = arg_traits<ARG...>::direct_any };
};

template<typename FUNC>
struct func_traits {
    typedef typename func_traits_impl<FUNC>::ret_type ret_type;
    enum { direct_any = func_traits_impl<FUNC>::direct_any };
    typedef typename func_traits_impl<FUNC>::arg_type arg_type;
};



template<typename RET, typename FUNC, bool direct_any_arg, std::size_t ...I>
struct call_tuple_t {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    typedef typename remove_reference_tuple<std::tuple<RET>>::type ret_type;
    
    static ret_type call(const FUNC &func, pm_any &arg) {
        func_arg_type new_arg(*reinterpret_cast<typename std::tuple_element<I, func_arg_type>::type *>(arg.tuple_element(I))...);
        arg.clear();
        return ret_type(func(std::get<I>(new_arg)...));
    }
};

template<typename RET, typename FUNC, std::size_t ...I>
struct call_tuple_t<RET, FUNC, true, I...> {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    typedef typename remove_reference_tuple<std::tuple<RET>>::type ret_type;

    static ret_type call(const FUNC &func, pm_any &arg) {
        return ret_type(func(arg));
    }
};

template<typename FUNC, std::size_t ...I>
struct call_tuple_t<void, FUNC, false, I...> {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    typedef std::tuple<> ret_type;

    static std::tuple<> call(const FUNC &func, pm_any &arg) {
        func_arg_type new_arg(*reinterpret_cast<typename std::tuple_element<I, func_arg_type>::type *>(arg.tuple_element(I))...);
        arg.clear();
        func(std::get<I>(new_arg)...);
        return std::tuple<>();
    }
};

template<typename FUNC, std::size_t ...I>
struct call_tuple_t<void, FUNC, true, I...> {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    typedef std::tuple<> ret_type;

    static std::tuple<> call(const FUNC &func, pm_any &arg) {
        func(arg);
        return std::tuple<>();
    }
};


template<typename RET>
struct call_tuple_ret_t {
    typedef typename remove_reference_tuple<std::tuple<RET>>::type ret_type;
};

template<>
struct call_tuple_ret_t<void> {
    typedef std::tuple<> ret_type;
};

template<typename FUNC, std::size_t ...I>
inline auto call_tuple_as_argument(const FUNC &func, pm_any &arg, const std::index_sequence<I...> &) 
    -> typename call_tuple_ret_t<typename func_traits<FUNC>::ret_type>::ret_type
{
    typedef typename func_traits<FUNC>::ret_type ret_type;
    enum { direct_any = func_traits<FUNC>::direct_any };

    return call_tuple_t<ret_type, FUNC, (direct_any ? true : false), I...>::call(func, arg);
}

template<typename FUNC>
inline bool verify_func_arg(const FUNC &func, pm_any &arg) {
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    type_tuple<func_arg_type> tuple_func;

    if (arg.tuple_size() < tuple_func.size_) {
        return false;
        //pm_throw(bad_any_cast(std::type_index(arg.type()), get_type_index(typeid(func_arg_type))));
    }

    for (size_t i = tuple_func.size_; i-- != 0; ) {
        if (arg.tuple_type(i) != tuple_func.tuple_type(i)
            && arg.tuple_type(i) != tuple_func.tuple_rcv_type(i)) {
            //printf("== %s ==> %s\n", arg.tuple_type(i).name(), tuple_func.tuple_type(i).name());
            return false;
            //pm_throw(bad_any_cast(arg.tuple_type(i), tuple_func.tuple_type(i)));
        }
    }

    return true;
}

template<typename FUNC>
inline auto call_func(const FUNC &func, pm_any &arg)
    -> typename call_tuple_ret_t<typename func_traits<FUNC>::ret_type>::ret_type
{
    typedef typename func_traits<FUNC>::arg_type func_arg_type;
    //type_tuple<func_arg_type> tuple_func;

    return call_tuple_as_argument(func, arg, std::make_index_sequence<type_tuple<func_arg_type>::size_>());
}

struct BypassAnyArg {};
struct Promise;

template< class T >
class pm_shared_ptr {
public:
    ~pm_shared_ptr() {
        pm_allocator::dec_ref(object_);
    }

    explicit pm_shared_ptr(T *object)
        : object_(object) {
    }

    explicit pm_shared_ptr()
        : object_(nullptr) {
    }

    pm_shared_ptr(pm_shared_ptr const &ptr)
        : object_(ptr.object_) {
        pm_allocator::add_ref(object_);
    }

    pm_shared_ptr &operator=(pm_shared_ptr const &ptr) {
        pm_shared_ptr(ptr).swap(*this);
        return *this;
    }

    bool operator==(pm_shared_ptr const &ptr) const {
        return object_ == ptr.object_;
    }

    bool operator!=(pm_shared_ptr const &ptr) const {
        return !(*this == ptr);
    }

    bool operator==(T const *ptr) const {
        return object_ == ptr;
    }

    bool operator!=(T const *ptr) const {
        return !(*this == ptr);
    }

    inline T *operator->() const {
        return object_;
    }

    inline T *obtain_rawptr() {
        pm_allocator::add_ref(object_);
        return object_;
    }

    inline void release_rawptr() {
        pm_allocator::dec_ref(object_);
    }

    void clear() {
        pm_shared_ptr().swap(*this);
    }

//private:

    inline void swap(pm_shared_ptr &ptr) {
        std::swap(object_, ptr.object_);
    }

    T *object_;
};

template< class T, class... Args >
inline pm_shared_ptr<T> pm_make_shared(Args&&... args) {
    return pm_shared_ptr<T>(pm_new<T>(args...));
}

template< class T, class B, class... Args >
inline pm_shared_ptr<B> pm_make_shared2(Args&&... args) {
    return pm_shared_ptr<B>(pm_new<T>(args...));
}


template<typename T>
class pm_shared_ptr_promise {
    typedef pm_shared_ptr_promise Defer;
public:
    ~pm_shared_ptr_promise() {
        pm_allocator::dec_ref(object_);
    }

    explicit pm_shared_ptr_promise(T *object)
        : object_(object) {
    }

    explicit pm_shared_ptr_promise()
        : object_(nullptr) {
    }

    pm_shared_ptr_promise(pm_shared_ptr_promise const &ptr)
        : object_(ptr.object_) {
        pm_allocator::add_ref(object_);
    }

    pm_shared_ptr_promise(pm_shared_ptr<T> const &ptr)
        : object_(ptr.object_) {
        pm_allocator::add_ref(object_);
    }

    Defer &operator=(Defer const &ptr) {
        Defer(ptr).swap(*this);
        return *this;
    }

    bool operator==(Defer const &ptr) const {
        return object_ == ptr.object_;
    }

    bool operator!=(Defer const &ptr) const {
        return !(*this == ptr);
    }

    bool operator==(T const *ptr) const {
        return object_ == ptr;
    }

    bool operator!=(T const *ptr) const {
        return !(*this == ptr);
    }

    inline T *operator->() const {
        return object_;
    }

    inline T *obtain_rawptr() {
        pm_allocator::add_ref(object_);
        return object_;
    }

    inline void release_rawptr() {
        pm_allocator::dec_ref(object_);
    }

    Defer find_pending() const {
        return object_->find_pending();
    }
    
    void reject_pending() {
        if(object_ != nullptr)
            object_->reject_pending();
    }

    void clear() {
        Defer().swap(*this);
    }

    template <typename ...RET_ARG>
    void resolve(const RET_ARG &... ret_arg) const {
        object_->template resolve<RET_ARG...>(ret_arg...);
    }
    void resolve(const pm_any &ret_arg) const {
        object_->resolve(ret_arg);
    }

    template <typename ...RET_ARG>
    void reject(const RET_ARG &... ret_arg) const {
        object_->template reject<RET_ARG...>(ret_arg...);
    }
    void reject(const pm_any &ret_arg) const {
        object_->reject(ret_arg);
    }
    void reject(const std::exception_ptr &ptr) const {
        object_->reject(ptr);
    }

    Defer then(Defer &promise) {
        return object_->then(promise);
    }

    Defer then(Defer&& promise) {
        return then(promise);
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

    template <typename FUNC_ON_FINALLY>
    Defer finally(FUNC_ON_FINALLY on_finally) const {
        return object_->template finally<FUNC_ON_FINALLY>(on_finally);
    }

    void call(Defer &promise) {
        object_->call(promise);
    }

    void dump() {
        object_->dump();
    }
private:
    inline void swap(Defer &ptr) {
        std::swap(object_, ptr.object_);
    }

    T *object_;
};

typedef pm_shared_ptr_promise<Promise> Defer;

typedef void(*FnSimple)();

template <typename RET, typename FUNC>
struct ResolveChecker;
template <typename RET, typename FUNC>
struct RejectChecker;
#ifndef PM_EMBED
template<typename ARG_TYPE, typename FUNC>
struct ExCheck;
#endif

#ifndef PM_EMBED
typedef std::function<void(Defer &d)> FnOnUncaughtException;
#endif

inline Defer newPromise(void);

struct PromiseCaller{
    virtual ~PromiseCaller(){};
    virtual Defer call(Defer &self, Promise *caller) = 0;
};

template <typename FUNC_ON_RESOLVED>
struct ResolvedCaller
    : public PromiseCaller{
    typedef typename func_traits<FUNC_ON_RESOLVED>::ret_type resolve_ret_type;
    FUNC_ON_RESOLVED on_resolved_;

    ResolvedCaller(const FUNC_ON_RESOLVED &on_resolved)
        : on_resolved_(on_resolved){}

    virtual Defer call(Defer &self, Promise *caller) {
        return ResolveChecker<resolve_ret_type, FUNC_ON_RESOLVED>::call(on_resolved_, self, caller);
    }
};

template <typename FUNC_ON_REJECTED>
struct RejectedCaller
    : public PromiseCaller{
    typedef typename func_traits<FUNC_ON_REJECTED>::ret_type reject_ret_type;
    FUNC_ON_REJECTED on_rejected_;

    RejectedCaller(const FUNC_ON_REJECTED &on_rejected)
        : on_rejected_(on_rejected){}

    virtual Defer call(Defer &self, Promise *caller) {
        return RejectChecker<reject_ret_type, FUNC_ON_REJECTED>::call(on_rejected_, self, caller);
    }
};

struct Promise {
    Defer next_;
    pm_stack::itr_t prev_;
    pm_any any_;
    PromiseCaller *resolved_;
    PromiseCaller *rejected_;

    enum status_t {
        kInit       = 0,
        kResolved   = 1,
        kRejected   = 2,
        kFinished   = 3
    };
    uint8_t status_      ;//: 2;

#ifdef PM_DEBUG
    uint32_t type_;
    uint32_t id_;
#endif

    Promise(const Promise &) = delete;
    explicit Promise()
        : next_(nullptr)
        , prev_(pm_stack::ptr_to_itr(nullptr))
        , resolved_(nullptr)
        , rejected_(nullptr)
        , status_(kInit)
#ifdef PM_DEBUG
        , type_(PM_TYPE_NONE)
        , id_(++(*dbg_promise_id()))
#endif
        {
        //printf("size promise = %d %d %d\n", (int)sizeof(*this), (int)sizeof(prev_), (int)sizeof(next_));
    }

    virtual ~Promise() {
        clear_func();
        if (next_.operator->()) {
#ifdef PM_MULTITHREAD
            std::lock_guard<std::recursive_mutex> lock(pm_mutex::get_mutex());
#endif
            next_->prev_ = pm_stack::ptr_to_itr(nullptr);
        }
#ifndef PM_EMBED
        else if (status_ == kRejected) {
            onUncaughtException(any_);
        }
#endif
    }

#ifndef PM_EMBED
    static FnOnUncaughtException *getUncaughtExceptionHandler() {
        static FnOnUncaughtException onUncaughtException = nullptr;
        return &onUncaughtException;
    }

    static void onUncaughtException(const pm_any &any) {
        FnOnUncaughtException *onUncaughtException = getUncaughtExceptionHandler();
        if (*onUncaughtException != nullptr) {
            Defer promise = newPromise();
            promise.reject(any);
            (*onUncaughtException)(promise);
            get_tail(promise.operator->())->fail([] {
            });
        }
    }

    static void handleUncaughtException(const FnOnUncaughtException &onUncaughtException) {
        (*getUncaughtExceptionHandler()) = onUncaughtException;
    }
#endif

    template <typename RET_ARG>
    void prepare_resolve(const RET_ARG &ret_arg) {
        if (status_ != kInit) return;
        status_ = kResolved;
        any_ = ret_arg;
    }

    template <typename ...RET_ARG>
    void resolve(const RET_ARG &... ret_arg) {
        typedef typename remove_reference_tuple<std::tuple<RET_ARG...>>::type arg_type;
        prepare_resolve(arg_type(ret_arg...));
        if(status_ == kResolved)
            call_next();
    }

    void resolve(const pm_any &ret_arg) {
        prepare_resolve(ret_arg);
        if (status_ == kResolved)
            call_next();
    }

    template <typename RET_ARG>
    void prepare_reject(const RET_ARG &ret_arg) {
        if (status_ != kInit) return;
        status_ = kRejected;
        any_ = ret_arg;
    }

    template <typename ...RET_ARG>
    void reject(const RET_ARG &...ret_arg) {
        typedef typename remove_reference_tuple<std::tuple<RET_ARG...>>::type arg_type;
        prepare_reject(arg_type(ret_arg...));
        if(status_ == kRejected)
            call_next();
    }

    void reject(const pm_any &ret_arg) {
        prepare_reject(ret_arg);
        if (status_ == kRejected)
            call_next();
    }

    void reject(const std::exception_ptr &ptr) {
        prepare_reject(ptr);
        if (status_ == kRejected)
            call_next();
    }

    Defer call_resolve(Defer &self, Promise *caller){
        if(resolved_ == nullptr){
            self->prepare_resolve(caller->any_);
            return self;
        }
#ifdef PM_MAX_CALL_LEN
        ++ (*dbg_promise_call_len());
        if((*dbg_promise_call_len()) > PM_MAX_CALL_LEN) pm_throw("PM_MAX_CALL_LEN");
#endif
        Defer ret = resolved_->call(self, caller);
#ifdef PM_MAX_CALL_LEN
        -- (*dbg_promise_call_len());
#endif
        if (ret != self) {
            joinDeferObject(self, ret);
            self->status_ = kFinished;
        }
        return ret;
    }

    Defer call_reject(Defer &self, Promise *caller){
        if(rejected_ == nullptr){
            self->prepare_reject(caller->any_);
            return self;
        }
#ifdef PM_MAX_CALL_LEN
        ++ (*dbg_promise_call_len());
        if((*dbg_promise_call_len()) > PM_MAX_CALL_LEN) pm_throw("PM_MAX_CALL_LEN");
#endif
        Defer ret = rejected_->call(self, caller);
#ifdef PM_MAX_CALL_LEN
        -- (*dbg_promise_call_len());
#endif
        if (ret != self) {
            joinDeferObject(self, ret);
            self->status_ = kFinished;
        }
        return ret;
    }

    void clear_func() {
        pm_delete(resolved_);
        resolved_ = nullptr;
        pm_delete(rejected_);
        rejected_ = nullptr;
    }

    template <typename FUNC>
    void run(FUNC func, Defer d) {
#ifndef PM_EMBED
        try {
            func(d);
        } catch(const bad_any_cast &ex) {
            d->reject(ex);
        } catch(...) {
            d->reject(std::current_exception());
        }
#else
        func(d);
#endif
    }
    
    Defer call_next() {
        uint8_t status = kInit;
        if (status_ == kResolved) {
            if (next_.operator->()) {
#ifdef PM_MULTITHREAD
                std::lock_guard<std::recursive_mutex> lock(pm_mutex::get_mutex());
#endif
                if (status_ == kResolved) {
                    status = status_;
                    status_ = kFinished;
                }
            }
        }
        else if (status_ == kRejected) {
            if (next_.operator->()) {
#ifdef PM_MULTITHREAD
                std::lock_guard<std::recursive_mutex> lock(pm_mutex::get_mutex());
#endif
                if (status_ == kRejected) {
                    status = status_;
                    status_ = kFinished;
                }
            }
        }

        if(status == kResolved){
            pm_allocator::add_ref(this);
            Defer d = next_->call_resolve(next_, this);
            this->any_.clear();
            next_->clear_func();
            if(d.operator->())
                d->call_next();
            //next_.clear();
            pm_allocator::dec_ref(this);
            return d;
        }
        else if(status == kRejected ) {
            pm_allocator::add_ref(this);
            Defer d =  next_->call_reject(next_, this);
            this->any_.clear();
            next_->clear_func();
            if (d.operator->())
                d->call_next();
            //next_.clear();
            pm_allocator::dec_ref(this);
            return d;
        }

        return next_;
    }


    Defer then_impl(PromiseCaller *resolved, PromiseCaller *rejected){
        Defer promise = newPromise();
        promise->resolved_ = resolved;
        promise->rejected_ = rejected;
        return then(promise);
    }

    Defer then(Defer &promise) {
        joinDeferObject(this, promise);
        //printf("2prev_ = %d %x %x\n", (int)promise->prev_, pm_stack::itr_to_ptr(promise->prev_), this);
        return call_next();
    }

    template <typename FUNC_ON_RESOLVED, typename FUNC_ON_REJECTED>
    Defer then(const FUNC_ON_RESOLVED &on_resolved, const FUNC_ON_REJECTED &on_rejected) {
        return then_impl(static_cast<PromiseCaller *>(pm_new<ResolvedCaller<FUNC_ON_RESOLVED>>(on_resolved)),
                         static_cast<PromiseCaller *>(pm_new<RejectedCaller<FUNC_ON_REJECTED>>(on_rejected)));
    }

    template <typename FUNC_ON_RESOLVED>
    Defer then(const FUNC_ON_RESOLVED &on_resolved) {
        return then_impl(static_cast<PromiseCaller *>(pm_new<ResolvedCaller<FUNC_ON_RESOLVED>>(on_resolved)),
                         static_cast<PromiseCaller *>(nullptr));
    }

    template <typename FUNC_ON_REJECTED>
    Defer fail(const FUNC_ON_REJECTED &on_rejected) {
        return then_impl(static_cast<PromiseCaller *>(nullptr),
                         static_cast<PromiseCaller *>(pm_new<RejectedCaller<FUNC_ON_REJECTED>>(on_rejected)));
    }

    template <typename FUNC_ON_ALWAYS>
    Defer always(const FUNC_ON_ALWAYS &on_always) {
        return then<FUNC_ON_ALWAYS, FUNC_ON_ALWAYS>(on_always, on_always);
    }

    template <typename FUNC_ON_FINALLY>
    Defer finally(const FUNC_ON_FINALLY &on_finally) {
        return then([on_finally](Promise *caller) -> BypassAnyArg {
            if(verify_func_arg(on_finally, caller->any_))
                call_func(on_finally, caller->any_);
            return BypassAnyArg();
        }, [on_finally](Defer &self, Promise *caller) -> BypassAnyArg {
#ifndef PM_EMBED
            typedef typename func_traits<FUNC_ON_FINALLY>::arg_type arg_type;
            if (caller->any_.type() == typeid(std::exception_ptr)) {
                ExCheck<arg_type, FUNC_ON_FINALLY>::call(on_finally, self, caller);
            }
            else {
                if (verify_func_arg(on_finally, caller->any_))
                    call_func(on_finally, caller->any_);
            }
#else
            if (verify_func_arg(on_finally, caller->any_))
                call_func(on_finally, caller->any_);
#endif
            return BypassAnyArg();
        });
    }

    void call(Defer &promise) {
        then([promise](Promise *caller) -> BypassAnyArg {
            promise.resolve(caller->any_);
            return BypassAnyArg();
        }, [promise](Defer &self, Promise *caller) -> BypassAnyArg {
            promise.reject(caller->any_);
            return BypassAnyArg();
        });
    }

    Defer find_pending() {
#ifdef PM_MULTITHREAD
        std::lock_guard<std::recursive_mutex> lock(pm_mutex::get_mutex());
#endif
        if (status_ == kInit) {
            Promise *p = this;
            Promise *prev = static_cast<Promise *>(pm_stack::itr_to_ptr(p->prev_));
            while (prev != nullptr) {
                if (prev->status_ != kInit)
                    return prev->next_;
                p = prev;
                prev = static_cast<Promise *>(pm_stack::itr_to_ptr(p->prev_));
            }

            pm_allocator::add_ref(p);
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

    void reject_pending(){
        Defer pending = find_pending();
        if(pending.operator->() != nullptr)
            pending.reject();
    }

    void dump() {
#ifdef PM_MULTITHREAD
        std::lock_guard<std::recursive_mutex> lock(pm_mutex::get_mutex());
#endif

        /* Check if there's any functions return null Defer object */
        pm_assert(next.operator->() != nullptr);

        Promise *head = get_head(this);
        printf("dump ");
        for (Promise *p = head; p != nullptr; p = p->next_.operator->()) {
            if(p == this)
                printf("*%p[%d] ", p, (int)p->status_);
            else
                printf("%p[%d] ", p, (int)p->status_);
        }
        printf("\n");
    }

private:
    static Promise *get_head(Promise *p){
        while(p){
            Promise *prev = static_cast<Promise *>(pm_stack::itr_to_ptr(p->prev_));
            if(prev == nullptr) break;
            p = prev;
        }
        return p;
    }
    static Promise *get_tail(Promise *p){
        while(p){
            Defer &next = p->next_;
            if(next.operator->() == nullptr) break;
            p = next.operator->();
        }
        return p;
    }
    
    
    static inline void joinDeferObject(Promise *self, Defer &next){
#ifdef PM_MULTITHREAD
        std::lock_guard<std::recursive_mutex> lock(pm_mutex::get_mutex());
#endif

        /* Check if there's any functions return null Defer object */
        pm_assert(next.operator->() != nullptr);

        Promise *head = get_head(next.operator->());
        Promise *tail = get_tail(next.operator->());

        if(self->next_.operator->()){
            self->next_->prev_ = pm_stack::ptr_to_itr(reinterpret_cast<void *>(tail));
            //printf("5prev_ = %d %x\n", (int)self->next_->prev_, pm_stack::itr_to_ptr(self->next_->prev_));
        }
        tail->next_ = self->next_;
        pm_allocator::add_ref(head);
        self->next_ = Defer(head);
        head->prev_ = pm_stack::ptr_to_itr(reinterpret_cast<void *>(self));
        //printf("6prev_ = %d %x\n", (int)next->prev_, pm_stack::itr_to_ptr(next->prev_));

#ifdef PM_DEBUG
        printf("debug promise id: ");
        for(Promise *p = head; p != nullptr; p = p->next_.operator->()) {
            printf("%d ", p->id_);
        }
        printf("\n");
#endif
    }

    static inline void joinDeferObject(Defer &self, Defer &next){
        joinDeferObject(self.operator->(), next);
    }
};


template <typename RET, typename FUNC>
struct ResolveChecker {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
#ifndef PM_EMBED
        try {
            if (verify_func_arg(func, caller->any_))
                self->prepare_resolve(call_func(func, caller->any_));
            else
                self->prepare_reject(caller->any_);
        } catch(...) {
            self->prepare_reject(std::current_exception());
        }
#else
        if (verify_func_arg(func, caller->any_))
            self->prepare_resolve(call_func(func, caller->any_));
        else
            self->prepare_reject(caller->any_);
#endif
        return self;
    }
};

template <typename FUNC>
struct ResolveChecker<Defer, FUNC> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
#ifndef PM_EMBED
        try {
            if (verify_func_arg(func, caller->any_)) {
                Defer ret = std::get<0>(call_func(func, caller->any_));
                return ret;
            }
            else {
                self->prepare_reject(caller->any_);
                return self;
            }
        } catch(...) {
            self->prepare_reject(std::current_exception());
            return self;
        }
#else
        if (verify_func_arg(func, caller->any_)) {
            Defer ret = std::get<0>(call_func(func, caller->any_));
            return ret;
        }
        else {
            self->prepare_reject(caller->any_);
            return self;
        }
#endif
    }
};

template <typename FUNC>
struct ResolveChecker<BypassAnyArg, FUNC> {
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
#ifndef PM_EMBED
        try {
            pm_any any = caller->any_;
            func(caller);
            self->prepare_resolve(any);
        }
        catch (...) {
            self->prepare_reject(std::current_exception());
        }
        return self;
#else
        pm_any any = caller->any_;
        func(caller);
        self->prepare_resolve(any);
        return self;
#endif
    }
};

template <typename RET>
struct ResolveChecker<RET, FnSimple> {
    static Defer call(const FnSimple &func, Defer &self, Promise *caller) {
#ifndef PM_EMBED
        try {
            if (func == nullptr)
                self->prepare_resolve(caller->any_);
            else if (verify_func_arg(func, caller->any_))
                self->prepare_resolve(call_func(func, caller->any_));
            else
                self->prepare_reject(caller->any_);
        } catch(...) {
            self->prepare_reject(std::current_exception());
        }
#else
        if (func == nullptr)
            self->prepare_resolve(caller->any_);
        else if (verify_func_arg(func, caller->any_))
            self->prepare_resolve(call_func(func, caller->any_));
        else
            self->prepare_reject(caller->any_);
#endif
        return self;
    }
};

#ifndef PM_EMBED
template<std::size_t ARG_SIZE, typename FUNC>
struct ExCheckTuple {
    static void *call(const FUNC &func, Defer &self, Promise *caller) {
        std::exception_ptr eptr = any_cast<std::exception_ptr>(caller->any_);
        throw eptr;
    }
};

template <typename FUNC>
struct ExCheckTuple<0, FUNC> {
    static auto call(const FUNC &func, Defer &self, Promise *caller) 
        -> typename call_tuple_ret_t<typename func_traits<FUNC>::ret_type>::ret_type {
        pm_any arg = std::tuple<>();
        caller->any_.clear();
        return call_func(func, arg);
    }
};

template <typename FUNC>
struct ExCheckTuple<1, FUNC> {
    typedef typename func_traits<FUNC>::arg_type arg_type;
    static auto call(const FUNC &func, Defer &self, Promise *caller)
        -> typename call_tuple_ret_t<typename func_traits<FUNC>::ret_type>::ret_type {
        std::exception_ptr eptr = any_cast<std::exception_ptr>(caller->any_);
        try {
            std::rethrow_exception(eptr);
        }
        catch (const typename std::tuple_element<0, arg_type>::type &ret_arg) {
            pm_any arg = arg_type(ret_arg);
            caller->any_.clear();
            return call_func(func, arg);
        }

        /* Will never run to here, just make the compile satisfied! */
        pm_any arg;
        return call_func(func, arg);
    }
};

template<typename ARG_TYPE, typename FUNC>
struct ExCheck:
    public ExCheckTuple<std::tuple_size<ARG_TYPE>::value, FUNC> {
};

template<typename FUNC>
struct ExCheck<pm_any, FUNC> {
    static auto call(const FUNC &func, Defer &self, Promise *caller) 
        -> typename call_tuple_ret_t<typename func_traits<FUNC>::ret_type>::ret_type {
        std::exception_ptr eptr = any_cast<std::exception_ptr>(caller->any_);
        try {
            std::rethrow_exception(eptr);
        }
        catch (pm_any &arg) {
            caller->any_.clear();
            return call_func(func, arg);
        }

        /* Will never run to here, just make the compile satisfied! */
        pm_any arg;
        return call_func(func, arg);
    }
};

#endif

template <typename RET, typename FUNC>
struct RejectChecker {
    typedef typename func_traits<FUNC>::arg_type arg_type;
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
#ifndef PM_EMBED
        try {
            if(caller->any_.type() == typeid(std::exception_ptr)){
                self->prepare_resolve(ExCheck<arg_type, FUNC>::call(func, self, caller));
            }
            else if (verify_func_arg(func, caller->any_))
                self->prepare_resolve(call_func(func, caller->any_));
            else
                self->prepare_reject(caller->any_);
        } catch(...) {
            self->prepare_reject(std::current_exception());
        }
#else
        if (verify_func_arg(func, caller->any_))
            self->prepare_resolve(call_func(func, caller->any_));
        else
            self->prepare_reject(caller->any_);
#endif
        return self;
    }
};

template <typename FUNC>
struct RejectChecker<Defer, FUNC> {
    typedef typename func_traits<FUNC>::arg_type arg_type;
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
#ifndef PM_EMBED
        try {
            if(caller->any_.type() == typeid(std::exception_ptr)){
                Defer ret = std::get<0>(ExCheck<arg_type, FUNC>::call(func, self, caller));
                return ret;
            }
            else if (verify_func_arg(func, caller->any_)) {
                Defer ret = std::get<0>(call_func(func, caller->any_));
                return ret;
            }
            else {
                self->prepare_reject(caller->any_);
                return self;
            }
        }
        catch(...) {
            self->prepare_reject(std::current_exception());
        }
        return self;
#else
        if (verify_func_arg(func, caller->any_)) {
            Defer ret = std::get<0>(call_func(func, caller->any_));
            return ret;
        }
        else {
            self->prepare_reject(caller->any_);
            return self;
        }
#endif
    }
};

template <typename FUNC>
struct RejectChecker<BypassAnyArg, FUNC> {
    typedef typename func_traits<FUNC>::arg_type arg_type;
    static Defer call(const FUNC &func, Defer &self, Promise *caller) {
#ifndef PM_EMBED
        try {
            pm_any any = caller->any_;
            func(self, caller);
            self->prepare_reject(any);
        }
        catch (...) {
            self->prepare_reject(std::current_exception());
        }
        return self;
#else
        pm_any any = caller->any_;
        func(self, caller);
        self->prepare_reject(any);
        return self;
#endif
    }
};

template <typename RET>
struct RejectChecker<RET, FnSimple> {
    static Defer call(const FnSimple &func, Defer &self, Promise *caller) {
#ifndef PM_EMBED
        try {
            if (func == nullptr)
                self->prepare_reject(caller->any_);
            else if (verify_func_arg(func, caller->any_))
                self->prepare_resolve(call_func(func, caller->any_));
            else
                self->prepare_reject(caller->any_);
        } catch(...) {
            self->prepare_reject(std::current_exception());
        }
#else
        if (func == nullptr)
            self->prepare_reject(caller->any_);
        else if (verify_func_arg(func, caller->any_))
            self->prepare_resolve(call_func(func, caller->any_));
        else
            self->prepare_reject(caller->any_);
#endif
        return self;
    }
};

inline Defer newPromise(){
    return Defer(pm_new<Promise>());
}

/* Create new promise object */
template <typename FUNC>
inline Defer newPromise(FUNC func) {
    Defer promise = newPromise();
    promise->run(func, promise);
    return promise;
}

/*
 * While loop func call resolved, 
 * It is not safe since the promise chain will become longer infinitely 
 * if the returned Defer object was obtained by other and not released.
 */
template <typename FUNC>
inline Defer doWhile_unsafe(FUNC func) {
    return newPromise(func).then([func]() {
        return doWhile_unsafe(func);
    });
}

/* While loop func call resolved */
template <typename FUNC>
inline Defer doWhile(FUNC func) {
    return newPromise([func](Defer d) {
        doWhile_unsafe(func).call(d);
    });
}

/* Return a rejected promise directly */
template <typename ...RET_ARG>
inline Defer reject(const RET_ARG &... ret_arg){
    return newPromise([=](Defer &d){ d.reject(ret_arg...); });
}

/* Return a resolved promise directly */
template <typename ...RET_ARG>
inline Defer resolve(const RET_ARG &... ret_arg){
    return newPromise([=](Defer &d){ d.resolve(ret_arg...); });
}

/* Returns a promise that resolves when all of the promises in the iterable
   argument have resolved, or rejects with the reason of the first passed
   promise that rejects. */
template <typename PROMISE_LIST>
inline Defer all(PROMISE_LIST &promise_list) {
    if(pm_size(promise_list) == 0){
        //return Promise::resolve<>()
    }

    size_t *finished = pm_new<size_t>(0);
    size_t *size = pm_new<size_t>(pm_size(promise_list));
    std::vector<pm_any> *ret_arr = pm_new<std::vector<pm_any>>();
    ret_arr->resize(*size);

    return newPromise([=](Defer &d) {
        size_t index = 0;
        for (auto defer : promise_list) {
            defer.then([=](pm_any &arg) {
                (*ret_arr)[index] = arg;
                if (++(*finished) >= *size) {
                    std::vector<pm_any> ret = *ret_arr;
                    pm_delete(finished);
                    pm_delete(size);
                    pm_delete(ret_arr);
                    d.resolve(ret);
                }
            }, [=](pm_any &arg) {
                pm_delete(finished);
                pm_delete(size);
                pm_delete(ret_arr);
                d.reject(arg);
            });

            ++index;
        }
    });
}

inline Defer all(std::initializer_list<Defer> promise_list) {
    return all<std::initializer_list<Defer>>(promise_list);
}

template <typename ... PROMISE_LIST>
inline Defer all(PROMISE_LIST ...promise_list) {
    return all({ promise_list ... });
}


/* returns a promise that resolves or rejects as soon as one of
the promises in the iterable resolves or rejects, with the value
or reason from that promise. */
template <typename PROMISE_LIST>
inline Defer race(PROMISE_LIST promise_list) {
    return newPromise([=](Defer d) {
        for (auto defer : promise_list) {
            defer.then([=](pm_any &arg) {
                d->resolve(arg);
            }, [=](pm_any &arg) {
                d->reject(arg);
            });
        }
    });
}

inline Defer race(std::initializer_list<Defer> promise_list) {
    return race<std::initializer_list<Defer>>(promise_list);
}

template <typename ... PROMISE_LIST>
inline Defer race(PROMISE_LIST ...promise_list) {
    return race({ promise_list ... });
}

#ifndef PM_EMBED
inline void handleUncaughtException(const FnOnUncaughtException &onUncaughtException) {
    Promise::handleUncaughtException(onUncaughtException);
}
#endif


}


#endif
