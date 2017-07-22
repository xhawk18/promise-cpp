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
struct remove_rcv {
    typedef typename std::remove_reference<T>::type r_type;
    typedef typename std::remove_cv<r_type>::type type;
};

template<typename T>
struct remove_rcv<const T *> {
    typedef typename remove_rcv<T *>::type type;
};

template<typename T>
struct remove_rcv<const T &> {
    typedef typename remove_rcv<T &>::type type;
};

template<typename T>
struct void_ptr_type {
    static void *cast(const T &t) {
        return reinterpret_cast<void *>(const_cast<char *>(&reinterpret_cast<const char &>(t)));
    }
};

template<typename T>
struct void_ptr_type<const T *> {
    typedef const T* PT;
    static void *cast(const PT &t) {
        return (void *)(&t);
    }
};

template<typename T>
void *void_ptr_cast(const T &t) {
    return void_ptr_type<T>::cast(t);
}

template<typename TUPLE, std::size_t SIZE>
struct offset_tuple_impl {
    template<size_t I_SIZE, std::size_t... I>
    struct offset_array {
        void *offsets_[I_SIZE];

        offset_array(const TUPLE *tuple)
            : offsets_{ void_ptr_cast(std::get<I>(*tuple))... } {
        }
    };

    template<std::size_t... I>
    static offset_array<SIZE, I...> get_array(const TUPLE *tuple, const std::index_sequence<I...> &) {
        return offset_array<SIZE, I...>(tuple);
    }

    decltype(get_array(nullptr, std::make_index_sequence<SIZE>())) value_;

    offset_tuple_impl(const TUPLE *tuple)
        : value_(get_array(tuple, std::make_index_sequence<SIZE>())) {
    }

    void *tuple_offset(size_t i) const {
        return value_.offsets_[i];
    }
};

template<typename TUPLE>
struct offset_tuple_impl<TUPLE, 0> {
    offset_tuple_impl(const TUPLE *tuple) {
    }
    void *tuple_offset(size_t i) const {
        return nullptr;
    }
};

template<typename NOT_TUPLE>
struct offset_tuple
    : public offset_tuple_impl<NOT_TUPLE, 0> {
    offset_tuple(const NOT_TUPLE *tuple)
        : offset_tuple_impl<NOT_TUPLE, 0>(tuple) {
    }
};

template<typename ...T>
struct offset_tuple<std::tuple<T...>>
    : public offset_tuple_impl<std::tuple<T...>, std::tuple_size<std::tuple<T...>>::value> {
    offset_tuple(const std::tuple<T...> *tuple)
        : offset_tuple_impl<std::tuple<T...>, std::tuple_size<std::tuple<T...>>::value>(tuple) {
    }
};

template<typename TUPLE, std::size_t SIZE>
struct type_tuple_impl {
    template<size_t I_SIZE, std::size_t... I>
    struct type_index_array {
        std::type_index types_[I_SIZE];
        std::type_index types_rcv_[I_SIZE];

        type_index_array()
            : types_{ get_type_index(typeid(typename std::tuple_element<I, TUPLE>::type))... }
            , types_rcv_{ get_type_index(typeid(typename remove_rcv<typename std::tuple_element<I, TUPLE>::type>::type))... } {
        }
    };

    template<std::size_t... I>
    static type_index_array<SIZE, I...> get_array(const std::index_sequence<I...> &) {
        return type_index_array<SIZE, I...>();
    }

    decltype(get_array(std::make_index_sequence<SIZE>())) value_;

    type_tuple_impl()
        : value_(get_array(std::make_index_sequence<SIZE>())) {
    }

    std::type_index tuple_type(size_t i) const {
        return value_.types_[i];
    }

    std::type_index tuple_rcv_type(size_t i) const {
        return value_.types_rcv_[i];
    }
};

template<typename TUPLE>
struct type_tuple_impl<TUPLE, 0> {
    static const std::size_t size_ = 0;
    std::type_index tuple_type(size_t i) const {
        return get_type_index(typeid(void));
    }
    std::type_index tuple_rcv_type(size_t i) const {
        return get_type_index(typeid(void));
    }
};

template<typename NOT_TUPLE>
struct type_tuple :
    public type_tuple_impl<void, 0> {
    static const std::size_t size_ = 0;
};

template<typename ...T>
struct type_tuple<std::tuple<T...>> :
    public type_tuple_impl<std::tuple<T...>, std::tuple_size<std::tuple<T...>>::value> {
    static const std::size_t size_ = std::tuple_size<std::tuple<T...>>::value;
};



class pm_any {
public: // structors
    pm_any()
        : content(0) {
    }

    template<typename ValueType>
    pm_any(const ValueType & value)
        : content(pm_new<holder<ValueType>>(value)) {
    }

    pm_any(const pm_any & other)
        : content(other.content ? other.content->clone() : 0) {
    }

    ~pm_any() {
        if (content != nullptr) {
            pm_delete(content);
        }
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

    std::size_t tuple_size() const {
        return content ? content->tuple_size() : 0;
    }

    std::type_index tuple_type(size_t i) const {
        return content ? content->tuple_type(i) : get_type_index(typeid(void));
    }

    void *tuple_element(size_t i) const {
        return content ? content->tuple_element(i) : nullptr;
    }

public: // types (public so any_cast can be non-friend)
    class placeholder {
    public: // structors
        virtual ~placeholder() {
        }

    public: // queries
        virtual const std::type_info & type() const = 0;
        virtual std::size_t tuple_size() const = 0;
        virtual std::type_index tuple_type(size_t i) const = 0;
        virtual void *tuple_element(size_t i) const = 0;

        virtual placeholder * clone() const = 0;
    };

    template<typename ValueType>
    class holder : public placeholder {
    public: // structors
        holder(const ValueType & value)
            : held(value)
            , type_tuple_()
            , offset_tuple_(&held) {
        }

    public: // queries
        virtual const std::type_info & type() const {
            return typeid(ValueType);
        }

        virtual std::size_t tuple_size() const {
            return type_tuple_.size_;
        }

        virtual std::type_index tuple_type(size_t i) const {
            return type_tuple_.tuple_type(i);
        }

        virtual void *tuple_element(size_t i) const {
            return offset_tuple_.tuple_offset(i);
        }

        virtual placeholder * clone() const {
            return pm_new<holder>(held);
        }
    public: // representation
        ValueType held;
        type_tuple<ValueType> type_tuple_;
        offset_tuple<ValueType> offset_tuple_;
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
    typedef typename std::remove_reference<ValueType>::type nonref;

    nonref * result = any_cast<nonref>(&operand);
    if (!result)
        pm_throw(bad_any_cast(get_type_index(operand.type()), get_type_index(typeid(ValueType))));
    return *result;
}

template<typename ValueType>
inline ValueType any_cast(const pm_any &operand) {
    typedef typename std::remove_reference<ValueType>::type nonref;
    return any_cast<const nonref &>(const_cast<pm_any &>(operand));
}

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


}
#endif
