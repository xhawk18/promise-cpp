#pragma once
#ifndef INC_COMPATIBILITY_HPP_
#define INC_COMPATIBILITY_HPP_

#include <iterator>
#include <type_traits>

namespace std {


#if __cplusplus < 201703L

template< class... >
using void_t = void;

#endif


#if __cplusplus < 202002L

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
