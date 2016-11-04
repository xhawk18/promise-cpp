#pragma once
#ifndef INC_FS_HPP_
#define INC_FS_HPP_

#include "promise.hpp"

namespace promise {

struct fs {
    enum mode_e {
#undef F_OK
#undef R_OK
#undef W_OK
#undef X_OK
        F_OK = 0,
        R_OK = 4,
        W_OK = 2,
        X_OK = 1
    };

    static Defer access(const char *path, mode_e mode = fs::F_OK);
    static int accessSync(const char *path, mode_e mode = fs::F_OK);
};

}
#endif
