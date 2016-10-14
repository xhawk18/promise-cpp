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
#include <algorithm>
#include <typeinfo>

namespace promise {

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
		class any
		{
		public: // structors

			any()
				: content(0)
			{
			}

			template<typename ValueType>
			any(const ValueType & value)
				: content(new holder<ValueType>(value))
			{
			}

			any(const any & other)
				: content(other.content ? other.content->clone() : 0)
			{
			}

			~any()
			{
				delete content;
			}

		public: // modifiers

			any & swap(any & rhs)
			{
				std::swap(content, rhs.content);
				return *this;
			}

			template<typename ValueType>
			any & operator=(const ValueType & rhs)
			{
				any(rhs).swap(*this);
				return *this;
			}

			any & operator=(const any & rhs)
			{
				any(rhs).swap(*this);
				return *this;
			}

		public: // queries

			bool empty() const
			{
				return !content;
			}

			const std::type_info & type() const
			{
				return content ? content->type() : typeid(void);
			}

		public: // types (public so any_cast can be non-friend)

			class placeholder
			{
			public: // structors

				virtual ~placeholder()
				{
				}

			public: // queries

				virtual const std::type_info & type() const = 0;

				virtual placeholder * clone() const = 0;

				virtual BaseT *get_pointer() = 0;

			};

			template<typename ValueType>
			class holder : public placeholder
			{
			public: // structors

				holder(const ValueType & value)
					: held(value)
				{
				}

			public: // queries

				virtual const std::type_info & type() const
				{
					return typeid(ValueType);
				}

				virtual placeholder * clone() const
				{
					return new holder(held);
				}

				inline virtual BaseT *get_pointer()
				{
					return static_cast<BaseT *>(&held);
				}

			public: // representation

				ValueType held;

			private: // intentionally left unimplemented
				holder & operator=(const holder &);
			};

		public: // representation (public so any_cast can be non-friend)

			placeholder * content;

			BaseT *base()
			{
				return content->get_pointer();
			}
			const BaseT *base() const
			{
				return content->get_pointer();
			}
		};

		class bad_any_cast : public std::bad_cast
		{
		public:
			virtual const char * what() const throw()
			{
				return "bad_any_cast: "
					"failed conversion using any_cast";
			}
		};

		template<typename ValueType, class BaseT>
		ValueType * any_cast(any<BaseT> * operand)
		{
			typedef typename any<BaseT>::template holder<ValueType> holder_t;
			return operand &&
				operand->type() == typeid(ValueType)
				? &static_cast<holder_t *>(operand->content)->held
				: 0;
		}

		template<typename ValueType, class BaseT>
		inline const ValueType * any_cast(const any<BaseT> * operand)
		{
			return any_cast<ValueType>(const_cast<any<BaseT> *>(operand));
		}

		template<typename ValueType, class BaseT>
		ValueType any_cast(any<BaseT> & operand)
		{
			typedef typename remove_reference<ValueType>::type nonref;

			nonref * result = any_cast<nonref>(&operand);
			if (!result)
				throw(bad_any_cast());
			return *result;
		}

		template<typename ValueType, class BaseT>
		inline ValueType any_cast(const any<BaseT> & operand)
		{
			typedef typename remove_reference<ValueType>::type nonref;
			return any_cast<const nonref &>(const_cast<any<BaseT> &>(operand));
		}

		// Note: The "unsafe" versions of any_cast are not part of the
		// public interface and may be removed at any time. They are
		// required where we know what type is stored in the any and can't
		// use typeid() comparison, e.g., when our types may travel across
		// different shared libraries.
		template<typename ValueType, class BaseT>
		inline ValueType * unsafe_any_cast(any<BaseT> * operand)
		{
			typedef typename any<BaseT>::template holder<ValueType> holder_t;
			return &static_cast<holder_t *>(operand->content)->held;
		}

		template<typename ValueType, class BaseT>
		inline const ValueType * unsafe_any_cast(const any<BaseT> * operand)
		{
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
    template <typename RET_ARG>
    void reject(const RET_ARG &ret_arg) const {
        object_->reject<RET_ARG>(ret_arg);
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


template <typename Promise, typename FUNC_ON_RESOLVE, typename FUNC_ON_REJECT>
struct PromiseEx 
    : public Promise {
    typedef typename GetRetArgType<FUNC_ON_RESOLVE>::ret_type resolve_ret_type;
    typedef typename GetRetArgType<FUNC_ON_RESOLVE>::arg_type resolve_arg_type;
    typedef typename GetRetArgType<FUNC_ON_REJECT>::ret_type reject_ret_type;
    typedef typename GetRetArgType<FUNC_ON_REJECT>::arg_type reject_arg_type;

    FUNC_ON_RESOLVE on_resolve_;
    FUNC_ON_REJECT on_reject_;
    
    PromiseEx(FUNC_ON_RESOLVE on_resolve, FUNC_ON_REJECT on_reject)
        : on_resolve_(on_resolve)
        , on_reject_(on_reject) {
    }
    
    virtual ~PromiseEx() {
    }
    
    virtual Defer call_resolve(Defer self, Promise *caller) {
        return ResolveChecker<PromiseEx, resolve_ret_type, FUNC_ON_RESOLVE, resolve_arg_type>::call(on_resolve_, self, caller);
    }
    virtual Defer call_reject(Defer self, Promise *caller) {
        return RejectChecker<PromiseEx, reject_ret_type, FUNC_ON_REJECT, reject_arg_type>::call(on_reject_, self, caller);
    }
};

struct Promise {
    int ref_count_;
    Defer next_;
    any<> ret_arg_;
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
    template <typename RET_ARG>
    void reject(const RET_ARG &ret_arg) {
        ret_arg_ = ret_arg;
        reject();
    }

    virtual Defer call_resolve(Defer self, Promise *caller) = 0;
    virtual Defer call_reject(Defer self, Promise *caller) = 0;

    template <typename FUNC>
    void run(FUNC func, Defer d) {
        try {
            func(d);
        } catch(const bad_any_cast &ex) {
            d->reject(ex);
        } catch(...) {
            d->reject();
        }
    }
    
    Defer call_next() {
        if(!next_.operator->()) {
        }
        else if(!is_called_ && is_resolved_) {
            is_called_ = true;
            Defer d = next_->call_resolve(next_, this);
            if(d.operator->())
                d->call_next();
            return d;
        }
        else if(!is_called_ && is_rejected_) {
            is_called_ = true;
            Defer d =  next_->call_reject(next_, this);
            if (d.operator->())
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
        try {
            self->resolve(func(any_cast<RET_ARG>(caller->ret_arg_)));
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct ResolveChecker<PROMISE_EX, Void, FUNC, RET_ARG> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            func(any_cast<RET_ARG>(caller->ret_arg_));
            self->resolve();
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};

template <typename PROMISE_EX, typename RET, typename FUNC>
struct ResolveChecker<PROMISE_EX, RET, FUNC, Void> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            self->resolve(func());
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC>
struct ResolveChecker<PROMISE_EX, Void, FUNC, Void> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            func();
            self->resolve();
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};


template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct ResolveChecker<PROMISE_EX, Defer, FUNC, RET_ARG> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            Defer ret = func(any_cast<RET_ARG>(caller->ret_arg_));
            ret->next_ = self->next_;
            return ret;
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
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
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};

template <typename PROMISE_EX>
struct ResolveChecker<PROMISE_EX, Void, FnSimple, Void> {
    static Defer call(FnSimple func, Defer self, Promise *caller) {
        try {
            if(func != NULL)
                (*func)();
            self->resolve();
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};


template <typename PROMISE_EX, typename RET, typename FUNC, typename RET_ARG>
struct RejectChecker {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            self->resolve(func(any_cast<RET_ARG>(caller->ret_arg_)));
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct RejectChecker<PROMISE_EX, Void, FUNC, RET_ARG> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            func(any_cast<RET_ARG>(caller->ret_arg_));
            self->resolve();
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};

template <typename PROMISE_EX, typename RET, typename FUNC>
struct RejectChecker<PROMISE_EX, RET, FUNC, Void> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            self->resolve(func());
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC>
struct RejectChecker<PROMISE_EX, Void, FUNC, Void> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            func();
            self->resolve();
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
        } catch(...) {
            self->reject();
        }
        return self;
    }
};


template <typename PROMISE_EX, typename FUNC, typename RET_ARG>
struct RejectChecker<PROMISE_EX, Defer, FUNC, RET_ARG> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            Defer ret = func(any_cast<RET_ARG>(caller->ret_arg_));
            ret->next_ = self->next_;
            return ret;
        }
        catch(const bad_any_cast &ex) {
            self->reject(ex);
        }
        catch(...) {
            self->reject();
        }
        return self;
    }
};

template <typename PROMISE_EX, typename FUNC>
struct RejectChecker<PROMISE_EX, Defer, FUNC, Void> {
    static Defer call(FUNC func, Defer self, Promise *caller) {
        try {
            Defer ret = func();
            ret->next_ = self->next_;
            return ret;
        }
        catch(const bad_any_cast &ex) {
            self->reject(ex);
        }
        catch(...) {
            self->reject();
        }
        return self;
    }
};

template <typename PROMISE_EX>
struct RejectChecker<PROMISE_EX, Void, FnSimple, Void> {
    static Defer call(FnSimple func, Defer self, Promise *caller) {
        try {
            if (func != NULL) {
                (*func)();
                self->resolve();
                return self;
            }
        } catch(const bad_any_cast &ex) {
            self->reject(ex);
            return self;
        } catch(...) {
            self->reject();
            return self;
        }

        self->ret_arg_.swap(caller->ret_arg_);
        self->reject();
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
