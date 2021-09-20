#pragma once
#ifndef INC_COMPATIBILITY_HPP_
#define INC_COMPATIBILITY_HPP_

namespace std {

#if __cplusplus < 202002L

template<class T>
struct remove_cvref {
    typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
};

#endif

}





template<typename T>
struct tuple_remove_cvref {
    using type = typename std::remove_cvref<T>::type;
};
template<typename ...T>
struct tuple_remove_cvref<std::tuple<T...>> {
    using type = std::tuple<typename std::remove_cvref<T>::type...>;
};


#endif
