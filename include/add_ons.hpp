#pragma once
#ifndef INC_COMPATIBILITY_HPP_
#define INC_COMPATIBILITY_HPP_

#include <iterator>
#include <type_traits>

namespace std {


#if (defined(_MSVC_LANG) && _MSVC_LANG < 201402L) || (!defined(_MSVC_LANG) && __cplusplus < 201402L)
template <size_t... Ints>
struct index_sequence {
    using type = index_sequence;
    using value_type = size_t;
    static constexpr std::size_t size() noexcept { return sizeof...(Ints); }
};

template <class Sequence1, class Sequence2>
struct _merge_and_renumber;

template <size_t... I1, size_t... I2>
struct _merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>>
    : index_sequence<I1..., (sizeof...(I1)+I2)...> {
};

template <size_t N>
struct make_index_sequence
    : _merge_and_renumber<typename make_index_sequence<N / 2>::type,
    typename make_index_sequence<N - N / 2>::type> {
};

template<> struct make_index_sequence<0> : index_sequence<> { };
template<> struct make_index_sequence<1> : index_sequence<0> { };
#endif //__cplusplus < 201402L


#if (defined(_MSVC_LANG) && _MSVC_LANG < 201703L) || (!defined(_MSVC_LANG) && __cplusplus < 201703L)

template< class... >
using void_t = void;

#endif //__cplusplus < 201703L


#if (defined(_MSVC_LANG) && _MSVC_LANG < 202002L) || (!defined(_MSVC_LANG) && __cplusplus < 202002L)

template<class T>
struct remove_cvref {
    typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
};

#endif //__cplusplus < 202002L

} //namespace std



namespace promise {

template<typename T>
struct tuple_remove_cvref {
    using type = typename std::remove_cvref<T>::type;
};
template<typename ...T>
struct tuple_remove_cvref<std::tuple<T...>> {
    using type = std::tuple<typename std::remove_cvref<T>::type...>;
};



template <typename T, typename = void>
struct is_iterable : std::false_type {};

// this gets used only when we can call std::begin() and std::end() on that type
template <typename T>
struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T>())),
    decltype(std::end(std::declval<T>()))
    >> : std::true_type {};

} //namespace promise

#endif
