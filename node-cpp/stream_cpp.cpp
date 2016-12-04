#include "uv.h"
#include "stream.hpp"

namespace promise {

Defer cuv_read(
    uv_stream_t *stream,
    Buffer buf,
    unsigned int nbufs) {

    return newPromise([stream, buf, nbufs](Defer d) {
        UvRead r({ pm_make_shared<UvReadArg>(buf, nbufs, stream), 0 });
        d->any_ = r;
        stream->read_arg = d.obtain_rawptr();

        uv_read_start(stream, [](uv_handle_t* handle,
            size_t suggested_size,
            uv_buf_t* uv_buf) {
            printf("suggested_size = %d, buf = %p\n", suggested_size, uv_buf);
            uv_stream_t *stream = reinterpret_cast<uv_stream_t *>(handle);
            Promise *promise = reinterpret_cast<Promise *>(stream->read_arg);
            UvRead &r = any_cast<UvRead &>(promise->any_);
            *uv_buf = uv_buf_init(&r.r_->buf_, r.r_->nbufs_);
        }, [](uv_stream_t* stream,
            ssize_t nread,
            const uv_buf_t* buf) {
            printf("nread = %d, buf = %p\n", nread, buf);
            Promise *promise = reinterpret_cast<Promise *>(stream->read_arg);
            UvRead &r = any_cast<UvRead &>(promise->any_);

            Defer d = Defer(reinterpret_cast<Promise *>(promise));
            d.release_rawptr();

            r.nread_ = nread;
            if (nread < 0) {
                if (nread != UV_EOF)
                    fprintf(stderr, "read error %s\n", uv_strerror(nread));
                d.reject(r);
            }
            else {
                d.resolve(r);
            }
        });
    });
}

Defer cuv_read(
    UvRead &r,
    Buffer buf,
    unsigned int nbufs) {

    return newPromise([r, buf, nbufs](Defer d) {
        r.r_->buf_ = buf;
        r.r_->nbufs_ = nbufs;
        r.r_->stream_->read_arg = reinterpret_cast<void *>(d.obtain_rawptr());
        d->any_ = r;
    });
}

void cuv_read_stop(Defer d) {
    d = d.find_pending();
    if (d.operator->()) {
        UvRead &r = any_cast<UvRead &>(d->any_);
        if (r.r_.operator->()) {
            r.r_->stop_read();
        }
        d.reject();
    }
}

Defer cuv_write(
    uv_stream_t* handle,
    const uv_buf_t bufs[],
    unsigned int nbufs) {

    struct UvWrite {
        Defer d_;
        uv_write_t uv_write_;
    };

    return newPromise([handle, bufs, nbufs](Defer d) {
        UvWrite *w = pm_new<UvWrite>(UvWrite({ d }));
        uv_write(&w->uv_write_, handle, bufs, nbufs, [](uv_write_t*uv_write, int status) {
            UvWrite *w = pm_container_of<UvWrite, uv_write_t>(uv_write, &UvWrite::uv_write_);
            Defer d = w->d_;
            pm_delete(w);
            d->resolve(status);
        });
    });
}

}
