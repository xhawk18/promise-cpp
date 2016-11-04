#pragma once
#ifndef INC_UV_DEFER_HPP_
#define INC_UV_DEFER_HPP_

#include "promise.hpp"

namespace promise {

template<typename UV_TYPE>
struct UvDefer : public UV_TYPE {
    Defer d_;

    UvDefer(Defer &d)
        : d_(d) {}

    void* operator new(size_t size){
        return pm_allocator<UvDefer>::obtain(size);
    }

        void operator delete(void *ptr) {
        pm_allocator<UvDefer>::release(ptr);
    }
};

}
#endif
