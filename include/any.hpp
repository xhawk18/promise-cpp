#pragma once
#ifndef INC_ANY_HPP_
#define INC_ANY_HPP_

#include <functional>
#include <typeindex>
#include <utility>
#include <stdexcept>
#include <tuple>
#include "call_traits.hpp"

namespace promise {

struct any {
    template<typename T>
    any(const T &value) : holder_(init(value)) {
    }

    any() : any(nullptr) {
    }

    template<typename T>
    T &cast() const {
        auto holder = holder_();
        if (std::get<1>(holder) != typeid(T))
            throw std::runtime_error("type not match");
        return reinterpret_cast<T &>(*reinterpret_cast<char *>(const_cast<void *>(std::get<0>(holder))));
    }

    const std::type_info &type() const {
        auto holder = holder_();
        return std::get<1>(holder);
    }

    any call(const any &arg) const {
        auto holder = holder_();
        return std::get<2>(holder)(arg);
    }

    bool empty() const {
        //auto holder = holder_();
        return type() == typeid(std::nullptr_t);
    }

private:
    using Holder = std::tuple<
                        const void *,                  // data pointer
                        const std::type_info &,        // data type
                        std::function<any(const any&)> // caller as function
                   >;
    std::function<Holder()> holder_;

    template<typename T>//, typename std::enable_if<!std::is_same<T, std::nullptr_t>::value>::type *dummy = 0>
    static std::function<Holder()> init(const T &t) {
        return [t]() -> Holder {
            return {
                reinterpret_cast<const void *>(&reinterpret_cast<const char &>(t)),
                typeid(T),
                [t](const any &arg) -> any {
                    return anyCall(t, arg);
                }
            };
        };
    }

    //static std::function<Holder()> init(const std::nullptr_t &t) {
    //    return nullptr;
    //}
};


//template<typename RET, typename FUNC, typename std::enable_if<std::is_same<FUNC,
//    std::function<any(const any &)>
//>::value>::type *dummy = nullptr>
//inline any stdFuncCall(const FUNC &func, const any &arg) {
//    return func(arg);
//}

template<typename RET, typename FUNC, typename std::enable_if<std::is_same<FUNC,
    std::function<RET()>
>::value && !std::is_same<RET, void>::value>::type *dummy = nullptr>
inline any stdFuncCall(const FUNC &func, const any &arg) {
    (void)arg;
    return func();
}

template<typename RET, typename FUNC, typename std::enable_if<std::is_same<FUNC,
    std::function<RET(const any &)>
>::value && !std::is_same<RET, void>::value>::type *dummy = nullptr>
inline any stdFuncCall(const FUNC &func, const any &arg) {
    return func(arg);
}

template<typename RET, typename FUNC, typename std::enable_if<std::is_same<FUNC,
    std::function<void(const any &)>
>::value>::type *dummy = nullptr>
inline any stdFuncCall(const FUNC &func, const any &arg) {
    func(arg);
    return nullptr;
}


template<typename RET, typename FUNC, typename std::enable_if<std::is_same<FUNC,
    std::function<void()>
>::value>::type *dummy = nullptr>
inline any stdFuncCall(const FUNC &func, const any &arg) {
    (void)arg;
    func();
    return nullptr;
}

template<typename FUNC>
inline any anyCall(const FUNC &func, const any &arg) {
    const auto &stdFunc = call_traits<FUNC>::to_std_function(func);
    if (!stdFunc)
        return nullptr;
    else
        return stdFuncCall<typename call_traits<FUNC>::result_type>(stdFunc, arg);
}


} // namespace promise
#endif

