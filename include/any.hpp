#pragma once
#ifndef INC_PM_ANY_HPP_
#define INC_PM_ANY_HPP_

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
#include <typeindex>
#include "add_ons.hpp"
#include "call_traits.hpp"

namespace promise {

// Any library
// See http://www.boost.org/libs/any for Documentation.
// what:  variant type any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

class pm_any;
template<typename ValueType>
inline ValueType any_cast(const pm_any &operand);

class pm_any {
public: // structors
    pm_any()
        : content(0) {
    }

    template<typename ValueType>
    pm_any(const ValueType & value)
        : content(new holder<ValueType>(value)) {
    }

    pm_any(const pm_any & other)
        : content(other.content ? other.content->clone() : 0) {
    }

    ~pm_any() {
        if (content != nullptr) {
            delete(content);
        }
    }

    pm_any call(const pm_any &arg) const {
        return content ? content->call(arg) : pm_any();
    }

    template<typename ValueType>
    inline ValueType cast() const {
        return any_cast<ValueType>(*this);
    }

public: // modifiers

    pm_any & swap(pm_any & rhs) {
        std::swap(content, rhs.content);
        return *this;
    }

    template<typename ValueType>
    pm_any & operator=(const ValueType & rhs) {
        pm_any(rhs).swap(*this);
        return *this;
    }

    pm_any & operator=(const pm_any & rhs) {
        pm_any(rhs).swap(*this);
        return *this;
    }

public: // queries
    bool empty() const {
        return !content;
    }
    
    void clear() {
        pm_any().swap(*this);
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
        virtual pm_any call(const pm_any &arg) const = 0;
    };

    template<typename ValueType>
    class holder : public placeholder {
    public: // structors
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

        virtual pm_any call(const pm_any &arg) const {
            return any_call(held, arg);
        }
    public: // representation
        ValueType held;
    private: // intentionally left unimplemented
        holder & operator=(const holder &);
    };

public: // representation (public so any_cast can be non-friend)
    placeholder * content;
};

class bad_any_cast : public std::bad_cast {
public:
    std::type_index from_;
    std::type_index to_;
    bad_any_cast(const std::type_index &from, const std::type_index &to)
        : from_(from)
        , to_(to) {
    }
    virtual const char * what() const throw() {
        return "bad_any_cast";
    }
};

template<typename ValueType>
ValueType * any_cast(pm_any *operand) {
    typedef typename pm_any::template holder<ValueType> holder_t;
    return operand &&
        operand->type() == typeid(ValueType)
        ? &static_cast<holder_t *>(operand->content)->held
        : 0;
}

template<typename ValueType>
inline const ValueType * any_cast(const pm_any *operand) {
    return any_cast<ValueType>(const_cast<pm_any *>(operand));
}

template<typename ValueType>
ValueType any_cast(pm_any & operand) {
    typedef typename std::remove_cvref<ValueType>::type nonref;

    nonref *result = any_cast<nonref>(&operand);
    if (!result)
        throw bad_any_cast(operand.type(), typeid(ValueType));
    return *result;
}

template<typename ValueType>
inline ValueType any_cast(const pm_any &operand) {
    typedef typename std::remove_cvref<ValueType>::type nonref;
    return any_cast<nonref &>(const_cast<pm_any &>(operand));
}




template<typename RET, typename FUNC, typename std::enable_if<std::is_same<FUNC,
    std::function<RET()>
>::value && !std::is_same<RET, void>::value>::type *dummy = nullptr>
inline pm_any any_call_stdFunc(const FUNC &func, const pm_any &arg) {
    (void)arg;
    return func();
}

template<typename RET, typename FUNC, typename std::enable_if<std::is_same<FUNC,
    std::function<RET(const pm_any &)>
>::value && !std::is_same<RET, void>::value>::type *dummy = nullptr>
inline pm_any any_call_stdFunc(const FUNC &func, const pm_any &arg) {
    return func(arg);
}

template<typename RET, typename FUNC, typename std::enable_if<std::is_same<FUNC,
    std::function<void(const pm_any &)>
>::value>::type *dummy = nullptr>
inline pm_any any_call_stdFunc(const FUNC &func, const pm_any &arg) {
    func(arg);
    return nullptr;
}


template<typename RET, typename FUNC, typename std::enable_if<std::is_same<FUNC,
    std::function<void()>
>::value>::type *dummy = nullptr>
inline pm_any any_call_stdFunc(const FUNC &func, const pm_any &arg) {
    (void)arg;
    func();
    return nullptr;
}


template<typename FUNC>
inline pm_any any_call(const FUNC &func, const pm_any &arg) {
    const auto &stdFunc = call_traits<FUNC>::to_std_function(func);
    if (!stdFunc)
        return pm_any();
    else
        return any_call_stdFunc<typename call_traits<FUNC>::result_type>(stdFunc, arg);
}


using any = pm_any;

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


}
#endif
