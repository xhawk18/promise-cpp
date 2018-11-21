#pragma once
#ifndef INC_PM_MISC_HPP_
#define INC_PM_MISC_HPP_

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

/* throw exception (override to avoid using exception when PM_EMBED is defined. */
template<typename T>
inline void pm_throw(const T &t){
#ifndef PM_EMBED
    throw t;
#else
    while(1);
#endif
}

/* same as std::size(...) in c++17 */
template <class C>
constexpr auto pm_size(const C& c) -> decltype(c.size()) {
    return c.size();
}
template <class T, std::size_t N>
constexpr std::size_t pm_size(const T(&array)[N]) noexcept {
    return N;
}

/* get offset of member in a structure */
template<class P, class M>
inline size_t pm_offsetof(const M P::*member) {
    return (size_t)&reinterpret_cast<char const&>((reinterpret_cast<P*>(0)->*member));
}

/* get structure address from a member */
template<class P, class M, class T>
inline P* pm_container_of(T* ptr, const M P::*member) {
    return reinterpret_cast<P*>(reinterpret_cast<char*>(ptr) - pm_offsetof(member));
}

#ifdef PM_MULTITHREAD

struct pm_mutex {
    static std::recursive_mutex &get_mutex() {
        static std::recursive_mutex mutex_;
        return mutex_;
    }
};

#endif

}
#endif
