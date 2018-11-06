#pragma once
#ifndef INC_PM_STD_ADDON_HPP_
#define INC_PM_STD_ADDON_HPP_

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

#if defined __GNUC__ && !defined __clang__
# if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 8)
#  error "Please use g++ 4.9.0 or higher"
# endif
#endif


#if defined __ARMCC_VERSION && __ARMCC_VERSION < 6000000
/* Missing headers for ARMCC */
/* Add on for std, used by arm cc */
namespace std {

template<typename T>
struct remove_reference{
    typedef T type;
};
template<typename T>
struct remove_reference<T &>{
    typedef T type;
};
template<typename T>
struct remove_const{
    typedef T type;
};
template<typename T>
struct remove_const<const T>{
    typedef T type;
};
template<typename T>
struct remove_volatile{
    typedef T type;
};
template<typename T>
struct remove_volatile<volatile T>{
    typedef T type;
};
template<typename T>
struct remove_cv{
    typedef typename remove_const<T>::type no_const_type;
    typedef typename remove_volatile<no_const_type>::type type;
};

template<class... Args>
struct tuple{
};

template<typename First, typename... Nexts>
struct tuple<First, Nexts...>{
    typedef First           first_type;
    typedef tuple<Nexts...> nexts_type;
    first_type first_;
    nexts_type nexts_;

    tuple(const first_type &first, const Nexts &... args)
        : first_(first)
        , nexts_(args...){
    }
};

template<>
struct tuple<>{
    tuple(){}
};

template<typename TUPLE>
struct tuple_size{
    static const size_t value = 1 + tuple_size<typename TUPLE::nexts_type>::value;
};
template<>
struct tuple_size<tuple<>>{
    static const size_t value = 0;
};

template<size_t I, typename TUPLE>
struct tuple_element{
    typedef typename tuple_element<I - 1, typename TUPLE::nexts_type>::type type;
};

template<typename TUPLE>
struct tuple_element<0, TUPLE>{
    typedef typename TUPLE::first_type type;
};

template<size_t I, typename TUPLE>
struct tuple_get {
    static const typename tuple_element<I, TUPLE>::type &get(const TUPLE &tuple) {
        return tuple_get<I - 1, typename TUPLE::nexts_type>::get(tuple.nexts_);
    }
    static typename tuple_element<I, TUPLE>::type &get(TUPLE &tuple) {
        return tuple_get<I - 1, typename TUPLE::nexts_type>::get(tuple.nexts_);
    }
};

template<typename TUPLE>
struct tuple_get<0, TUPLE> {
    static const typename tuple_element<0, TUPLE>::type &get(const TUPLE &tuple) {
        return tuple.first_;
    }
    static typename tuple_element<0, TUPLE>::type &get(TUPLE &tuple) {
        return tuple.first_;
    }
};

template<size_t I, typename TUPLE>
inline const typename tuple_element<I, TUPLE>::type &get(const TUPLE &tuple){
    (void)tuple;
    return tuple_get<I, TUPLE>::get(tuple);
}
template<size_t I, typename TUPLE>
inline typename tuple_element<I, TUPLE>::type &get(TUPLE &tuple) {
    (void)tuple;
    return tuple_get<I, TUPLE>::get(tuple);
}


typedef const std::type_info *type_index;
inline type_index get_type_index(const std::type_info &info){
    return type_index(&info);
}

} //namespace std

#else

#include <tuple>
#include <typeindex>
#include <type_traits>

inline std::type_index get_type_index(const std::type_info &info){
    return std::type_index(info);
}
#endif



#if (defined(_MSVC_LANG) && _MSVC_LANG < 201402L) || (!defined(_MSVC_LANG) && __cplusplus < 201402L)
namespace std {

template <size_t... Ints>
struct index_sequence
{
    using type = index_sequence;
    using value_type = size_t;
    static constexpr std::size_t size() noexcept { return sizeof...(Ints); }
};

template <class Sequence1, class Sequence2>
struct _merge_and_renumber;

template <size_t... I1, size_t... I2>
struct _merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>>
    : index_sequence<I1..., (sizeof...(I1)+I2)...>
{ };

template <size_t N>
struct make_index_sequence
    : _merge_and_renumber<typename make_index_sequence<N / 2>::type,
    typename make_index_sequence<N - N / 2>::type>
{ };

template<> struct make_index_sequence<0> : index_sequence<> { };
template<> struct make_index_sequence<1> : index_sequence<0> { };

} //namespace std
#endif

 
#endif
