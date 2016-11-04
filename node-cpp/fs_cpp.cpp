#include "uv.h"
#include "uv_defer.hpp"
#include "fs.hpp"

namespace promise {

using UvFs = UvDefer<uv_fs_t>;

Defer fs::access(const char *path, fs::mode_e mode){ 
    return newPromise([=](Defer &d) {
        UvFs *req_ = new UvFs(d);
        uv_loop_t *loop = uv_default_loop();
        uv_fs_access(loop,
            static_cast<uv_fs_t *>(req_),
            path,
            (int)mode,
            [](uv_fs_t *req) {
                uv_fs_req_cleanup(req);
                UvFs *req_ = static_cast<UvFs *>(req);
                Defer d = req_->d_;
                ssize_t result = req->result;
                delete req_;

                if (result == 0)
                    d.resolve();
                else
                    d.reject((int)result);
            });
    });
}

int fs::accessSync(const char *path, fs::mode_e mode) {
    uv_loop_t *loop = uv_default_loop();
    uv_fs_t req;
    int ret = uv_fs_access(loop, &req, path, (int)mode, nullptr);
    uv_fs_req_cleanup(&req);
    return (int)req.result;
}

}
