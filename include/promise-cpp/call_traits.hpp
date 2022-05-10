/************************************************************************************
*                                                                                   *
*   Copyright (c) 2021 xhchen8018@163.com                                           *
*                                                                                   *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining           *
*   a copy of this software and associated documentation files (the "Software"),    *
*   to deal in the Software without restriction, including without limitation       *
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,        *
*   and/or sell copies of the Software, and to permit persons to whom the           *
*   Software is furnished to do so, subject to the following conditions:            *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

/* 将任意可执行函数或对象的可调用性质
 *
 *  判断类型T是否可被调用
 *    call_traits<T>::is_callable 
 *
 *  将对象t转为std::function
 *    call_traits<decltype(t)>::to_std_function(t)
 *    如果t不可调用，会返回一个空函数
 */
 
#pragma once
#ifndef INC_CALL_TRAITS_HPP_
#define INC_CALL_TRAITS_HPP_

#include <type_traits>
#include <functional>
#include <tuple>

namespace promise {

template<typename T,
         bool is_basic_type = (std::is_void<T>::value
                            || std::is_fundamental<T>::value
                            || (std::is_pointer<T>::value && !std::is_function<typename std::remove_pointer<T>::type>::value)
                            || std::is_union<T>::value
                            || std::is_enum<T>::value
                            || std::is_array<T>::value) && !std::is_function<T>::value>
struct call_traits_impl;

template<typename T>
struct has_operator_parentheses {
private:
    struct Fallback { void operator()(); };
    struct Derived : T, Fallback { };

    template<typename U, U> struct Check;

    template<typename>
    static std::true_type test(...);

    template<typename C>
    static std::false_type test(Check<void (Fallback::*)(), &C::operator()>*);

public:
    typedef decltype(test<Derived>(nullptr)) type;
};

template<typename T, bool has_operator_parentheses>
struct operator_parentheses_traits {
    typedef std::function<void(void)> fun_type;
    typedef void result_type;
    typedef std::tuple<> argument_type;
    
    static fun_type to_std_function(const T &t) {
        (void)t;
        return nullptr;
    }
};

template<typename FUNCTOR>
struct operator_parentheses_traits<FUNCTOR, true> {
private:
    typedef decltype(&FUNCTOR::operator()) callable_type;
    typedef call_traits_impl<callable_type> the_type;
public:
    typedef typename the_type::fun_type fun_type;
    typedef typename the_type::result_type result_type;
    typedef typename the_type::argument_type argument_type;

    static fun_type to_std_function(const FUNCTOR &functor) {
        if (is_std_function<FUNCTOR>::value) {
            // on windows, FUNCTOR is not same as std::function<...>, must return functor directly.
            return functor;
        }
        else {
            return call_traits_impl<callable_type>::to_std_function(const_cast<FUNCTOR &>(functor), &FUNCTOR::operator());
        }
    }
};

//template<typename T, bool is_basic_type>
//struct call_traits_impl {
//    typedef call_traits_impl<T, is_fundamental, is_pointer> the_type;
//    static const bool is_callable = the_type::is_callable;
//    typedef typename the_type::fun_type fun_type;

//    static fun_type to_std_function(const T &t) {
//        return the_type::to_std_function(t);
//    }
//};

template<typename T>
struct call_traits_impl<T, true> {
    static const bool is_callable = false;
    typedef std::function<void(void)> fun_type;
    typedef void result_type;
    typedef std::tuple<> argument_type;

    static fun_type to_std_function(const T &t) {
        (void)t;
        return nullptr;
    }
};


template<typename RET, class T, typename ...ARG>
struct call_traits_impl<RET(T::*)(ARG...), false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;

    static fun_type to_std_function(T &obj, RET(T::*func)(ARG...)) {
        return [obj, func](ARG...arg) -> RET {
            return (const_cast<typename std::remove_const<T>::type &>(obj).*func)(arg...);
        };
    }

    // Never called, to make compiler happy
    static fun_type to_std_function(RET(T::*)(ARG...)) {
        return nullptr;
    }
};

template<typename RET, class T, typename ...ARG>
struct call_traits_impl<RET(T::*)(ARG...) const, false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;

    static fun_type to_std_function(T &obj, RET(T:: *func)(ARG...) const) {
        return [obj, func](ARG...arg) -> RET {
            return (obj.*func)(arg...);
        };
    }

    // Never called, to make compiler happy
    static fun_type to_std_function(RET(T:: *)(ARG...)) {
        return nullptr;
    }
};

template<typename RET, typename ...ARG>
struct call_traits_impl<RET(*)(ARG...), false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;

    static fun_type to_std_function(RET(*func)(ARG...)) {
        return func;
    }
};

template<typename RET, typename ...ARG>
struct call_traits_impl<RET(ARG...), false> {
    static const bool is_callable = true;
    typedef std::function<RET(ARG...)> fun_type;
    typedef RET result_type;
    typedef std::tuple<ARG...> argument_type;

    static fun_type to_std_function(RET(*func)(ARG...)) {
        return func;
    }
};

template<typename T>
struct call_traits_impl<T, false> {
    static const bool is_callable = has_operator_parentheses<T>::type::value;
private:
    typedef operator_parentheses_traits<T, is_callable> the_type;
public:
    typedef typename the_type::fun_type fun_type;
    typedef typename the_type::result_type result_type;
    typedef typename the_type::argument_type argument_type;

    static fun_type to_std_function(const T &t) {
        return the_type::to_std_function(t);
    }
};

template<typename T>
struct call_traits {
private:
    using RawT = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
    typedef call_traits_impl<RawT> the_type;
public:
    static const bool is_callable = the_type::is_callable;
    typedef typename the_type::fun_type fun_type;
    typedef typename the_type::result_type result_type;
    typedef typename the_type::argument_type argument_type;	

    static fun_type to_std_function(const T &t) {
        return the_type::to_std_function(t);
    }
};

#if 0 // Not used currently

template <typename ...P>
struct subst_gather {};

template <bool match, size_t lastN, template <typename ...> class Obj, typename T, typename ...P>
struct subst_matcher;

template <size_t lastN, template <typename ...> class Obj, typename ...P1, typename T, typename ...P2>
struct subst_matcher<false, lastN, Obj, subst_gather<P1...>, T, P2...>
{
    using type = typename subst_matcher<
        (lastN >= std::tuple_size<std::tuple<P2...>>::value),
        lastN, Obj, subst_gather<P1..., T>, P2...>::type;
};

template <size_t lastN, template <typename ...> class Obj, typename ...P1, typename ...P2>
struct subst_matcher<true, lastN, Obj, subst_gather<P1...>, P2...>
{
    using type = Obj<P1...>;
};

template <size_t lastN, template <typename ...> class Obj, typename ...P>
struct subst_all_but_last {
    using type = typename subst_matcher<
        (lastN >= std::tuple_size<std::tuple<P...>>::value),
        lastN, Obj, subst_gather<>, P...>::type;
};


template <class... ARGS>
struct call_with_more_args_helper {
    template<typename FUNC, typename ...MORE>
    static auto call(FUNC &&func, ARGS&&...args, MORE&&...) -> typename call_traits<FUNC>::result_type {
        return func(args...);
    }
};


template<typename FUNC, typename ...ARGS>
auto call_with_more_args(FUNC &&func, ARGS&&...args) -> typename call_traits<FUNC>::result_type {
    using Type = typename subst_all_but_last<
        (std::tuple_size<std::tuple<ARGS...>>::value - std::tuple_size<typename call_traits<FUNC>::argument_type>::value),
        call_with_more_args_helper, ARGS...>::type;
    return Type::call(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

#endif

}
#endif
