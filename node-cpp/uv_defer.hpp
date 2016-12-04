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

};

}
#endif
