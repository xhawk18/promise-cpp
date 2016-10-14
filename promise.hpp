/*
 * Promise API implemented by cpp as Javascript promise style 
 *
 * Copyright (c) 2012, xhawk18
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

#include <functional>
#include <memory>
#include "ll_any.hpp"

namespace promise {

struct Promise;

template< class T >
class shared_ptr {
    typedef shared_ptr<Promise> Defer;
public:
    virtual ~shared_ptr() {
        if(object_ != NULL) {
            --object_->ref_count_;
            if(object_->ref_count_ == 0)
                delete object_;
        }
    }

    explicit shared_ptr(T *object)
        : object_(object) {
        if(object_ != NULL)
            ++object_->ref_count_;
    }
    
    explicit shared_ptr()
        : object_(NULL) {
    }

    shared_ptr(shared_ptr const &ptr)
        : object_(ptr.object_) {
        if(object_ != NULL)
            ++object_->ref_count_;
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

    T *operator->() {
        return object_;
    }
    T *operator->() const {
        return object_;
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
	template <typename FUNC_ON_RESOLVE, typename FUNC_ON_REJECT>
    Defer then(FUNC_ON_RESOLVE on_resolve, FUNC_ON_REJECT on_reject) const {
        return object_->then<FUNC_ON_RESOLVE, FUNC_ON_REJECT>(on_resolve, on_reject);
    }

    template <typename FUNC_ON_RESOLVE>
    Defer then(FUNC_ON_RESOLVE on_resolve) const {
        return object_->then<FUNC_ON_RESOLVE>(on_resolve);
    }

    template <typename FUNC_ON_REJECT>
    Defer fail(FUNC_ON_REJECT on_reject) const {
        return object_->fail<FUNC_ON_REJECT>(on_reject);
    }
    
    template <typename FUNC_ON_ALWAYS>
    Defer always(FUNC_ON_ALWAYS on_always) const {
        return object_->always<FUNC_ON_ALWAYS>(on_always);
    }   
private:
    inline void swap(shared_ptr &ptr) {
        std::swap(object_, ptr.object_);
    }

    T *object_;
};

typedef std::shared_ptr<Promise> Defer;

typedef void(*FnSimple)();

struct Void {};

template <typename PROMISE_EX, typename RET, typename FUNC, typename RET_ARG>
struct ResolveChecker;
template <typename RET, typename FUNC>
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


template <typename Promise, typename FUNC_ON_RESOLVE, typename FUNC_ON_REJECT>
struct PromiseEx 
    : public Promise {
    typedef typename GetRetArgType<FUNC_ON_RESOLVE>::ret_type ret_type;
    typedef typename GetRetArgType<FUNC_ON_RESOLVE>::arg_type arg_type;
	typedef typename GetRetArgType<FUNC_ON_REJECT>::ret_type reject_ret_type;

    FUNC_ON_RESOLVE on_resolve_;
    FUNC_ON_REJECT on_reject_;
    
    PromiseEx(FUNC_ON_RESOLVE on_resolve, FUNC_ON_REJECT on_reject)
        : on_resolve_(on_resolve)
        , on_reject_(on_reject) {
    }
    
    virtual ~PromiseEx() {
    }
    
    virtual Defer call_resolve(Defer self, Promise *caller) {
        return ResolveChecker<PromiseEx, ret_type, FUNC_ON_RESOLVE, arg_type>::call(on_resolve_, self, caller);
    }
    virtual Defer call_reject(Defer self, Promise *caller) {
        return RejectChecker<reject_ret_type, FUNC_ON_REJECT>::call(on_reject_, self);
    }
};

struct Promise {
    int ref_count_;
    Defer next_;
    ll::any<> ret_arg_;
    bool is_resolved_;
    bool is_rejected_;
    bool is_called_;
    
    Promise(const Promise &) = delete;
    explicit Promise()
        : ref_count_(0)
        , next_(NULL)
        , is_resolved_(false)
        , is_rejected_(false)
        , is_called_(false) {
    }
    
    virtual ~Promise() {
    }
    
    void resolve() {
        if(is_resolved_ || is_rejected_)
            return;
        is_resolved_ = true;
        call_next();
    }
    template <typename RET_ARG>
    void resolve(const RET_ARG &ret_arg) {
        ret_arg_ = ret_arg;
        resolve();
    }
    void reject() {
        if(is_resolved_ || is_rejected_)
            return;
        is_rejected_ = true;
        call_next();
    }

    virtual Defer call_resolve(Defer self, Promise *caller) = 0;
    virtual Defer call_reject(Defer self, Promise *caller) = 0;

    template <typename FUNC>
    void run(FUNC func, Defer d) {
        try {
            func(d);
            return;
        } catch(...) {}
        d->reject();
    }
    
    Defer call_next() {
        if(!next_.get()) {
        }
        else if(!is_called_ && is_resolved_) {
            is_called_ = true;
            Defer d = next_->call_resolve(next_, this);
            if(d.get())
                d->call_next();
            return d;
        }
        else if(!is_called_ && is_rejected_) {
            is_called_ = true;
            Defer d =  next_->call_reject(next_, this);
            if (d.get())
                d->call_next();
            return d;
        }

        return next_;
    }

    template <typename FUNC_ON_RESOLVE, typename FUNC_ON_REJECT>
    Defer then(FUNC_ON_RESOLVE on_resolve, FUNC_ON_REJECT on_reject) {
        Defer promise(new PromiseEx<Promise, FUNC_ON_RESOLVE, FUNC_ON_REJECT>(on_resolve, on_reject));
        next_ = promise;
        return call_next();
    }

    template <typename FUNC_ON_RESOLVE>
    Defer then(FUNC_ON_RESOLVE on_resolve) {
        return then<FUNC_ON_RESOLVE, FnSimple>(on_resolve, NULL);
    }

    template <typename FUNC_ON_REJECT>
    Defer fail(FUNC_ON_REJECT on_reject) {
        return then<FnSimple, FUNC_ON_REJECT>(NULL, on_reject);
    }
    
    template <typename FUNC_ON_ALWAYS>
    Defer always(FUNC_ON_ALWAYS on_always) {
        return then<FUNC_ON_ALWAYS, FUNC_ON_ALWAYS>(on_always, on_always);
    }    
};

template <typename PROMISE_EX, typename RET, typename FUNC, typename RET_ARG>
struct ResolveChecker {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        bool success = false;
        try {
            RET ret = func(ll::any_cast<RET_ARG>(caller->ret_arg_));
            success = true;
            self->resolve(ret);
        } catch(...) {
            if (success) throw;
        }
        if(!success) self->reject();
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct ResolveChecker<PROMISE_EX, Void, FUNC, RET_ARG> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        bool success = false;
        try {
            func(ll::any_cast<RET_ARG>(caller->ret_arg_));
            success = true;
        }
        catch (...) {}
        if (success) self->resolve();
        else        self->reject();
        return self;
    }
};

template <typename PROMISE_EX, typename RET, typename FUNC>
struct ResolveChecker<PROMISE_EX, RET, FUNC, Void> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        bool success = false;
        try {
            RET ret = func();
            success = true;
            self->resolve(ret);
        } catch(...) {
            if (success) throw;
        }
        if(!success) self->reject();
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC>
struct ResolveChecker<PROMISE_EX, Void, FUNC, Void> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        bool success = false;
        try {
            func();
            success = true;
        }
        catch (...) {}
        if (success) self->resolve();
        else        self->reject();
        return self;
    }
};


template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct ResolveChecker<PROMISE_EX, Defer, FUNC, RET_ARG> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            Defer ret = func(ll::any_cast<RET_ARG>(caller->ret_arg_));
            ret->next_ = self->next_;
            return ret;
        } catch(...) {}

        self->reject();
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC>
struct ResolveChecker<PROMISE_EX, Defer, FUNC, Void> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            Defer ret = func();
            ret->next_ = self->next_;
            return ret;
        } catch(...) {}

        self->reject();
        return self;
    }
};

template <typename PROMISE_EX>
struct ResolveChecker<PROMISE_EX, Void, FnSimple, Void> {
	static Defer call(FnSimple func, Defer self, Promise *caller) {
		bool success = false;
		try {
			if(func != NULL)
				(*func)();
			success = true;
		}
		catch (...) {}
		if (success) self->resolve();
		else        self->reject();
		return self;
	}
};



template <typename RET, typename FUNC>
struct RejectChecker {
    static Defer call(FUNC func, Defer self) {
        bool success = false;
        try {
            func();
            success = true;
        } catch(...) {}
        if(success) self->resolve();
        else        self->reject();
        return self;
    }
};

template <typename FUNC>
struct RejectChecker<Defer, FUNC> {
    static Defer call(FUNC func, Defer self) {
        try {
            Defer ret = func();
            ret->next_ = self->next_;
            return ret;
        } catch(...) {}
        
        self->reject();
        return self;
    }
};

template <>
struct RejectChecker<Void, FnSimple> {
	static Defer call(FnSimple func, Defer self) {
		bool success = false;
		try {
			if(func != NULL) {
				(*func)();
				success = true;
			}
		}
		catch (...) {}
		if (success) self->resolve();
		else        self->reject();
		return self;
	}
};


template <typename FUNC>
Defer newPromise(FUNC func) {
    Defer promise(new PromiseEx<Promise, FnSimple, FnSimple>(NULL, NULL));
    promise->run(func, promise);
    return promise;
}


}
#endif
