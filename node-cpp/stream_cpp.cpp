#include "uv.h"
#include "stream.hpp"

namespace promise {

Defer cuv_read(
    uv_stream_t *stream,
    Buffer buf,
    unsigned int nbufs) {

    return newPromise([stream, buf, nbufs](Defer d) {
        d->ret_arg_ = shared_ptr<UvReadArg>(new UvReadArg(buf, nbufs, stream));
        d.add_ref();
        stream->read_arg = reinterpret_cast<void *>(d.operator->());
        uv_read_start(stream, [](uv_handle_t* handle,
            size_t suggested_size,
            uv_buf_t* uv_buf) {
            printf("suggested_size = %d, buf = %p\n", suggested_size, uv_buf);
            uv_stream_t *stream = reinterpret_cast<uv_stream_t *>(handle);
            Promise *promise = reinterpret_cast<Promise *>(stream->read_arg);
            shared_ptr<UvReadArg> &r = any_cast<shared_ptr<UvReadArg> &>(promise->ret_arg_);
            *uv_buf = uv_buf_init(&r->buf_, r->nbufs_);
        }, [](uv_stream_t* stream,
            ssize_t nread,
            const uv_buf_t* buf) {
            printf("nread = %d, buf = %p\n", nread, buf);
            Promise *promise = reinterpret_cast<Promise *>(stream->read_arg);
            shared_ptr<UvReadArg> &r = any_cast<shared_ptr<UvReadArg> &>(promise->ret_arg_);

            Defer d = Defer(reinterpret_cast<Promise *>(promise));
            d.dec_ref();

            if (nread < 0) {
                if (nread != UV_EOF)
                    fprintf(stderr, "read error %s\n", uv_strerror(nread));
                d.reject(UvRead({ r, nread }));
            }
            else {
                d.resolve(UvRead({ r, nread }));
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
        r.r_->stream_->read_arg = reinterpret_cast<void *>(d.operator->());
        d->ret_arg_ = r.r_;
        d.add_ref();
    });
}

Defer cuv_write(
    uv_stream_t* handle,
    const uv_buf_t bufs[],
    unsigned int nbufs) {

    struct UvWrite {
        Defer d_;
        uv_write_t uv_write_;
        void* operator new(size_t size){
            return allocator<UvWrite>::obtain(size);
        }
            void operator delete(void *ptr) {
            allocator<UvWrite>::release(ptr);
        }
    };

    return newPromise([handle, bufs, nbufs](Defer d) {
        UvWrite *w = new UvWrite({ d });
        uv_write(&w->uv_write_, handle, bufs, nbufs, [](uv_write_t*uv_write, int status) {
            UvWrite *w = pm_container_of<UvWrite, uv_write_t>(uv_write, &UvWrite::uv_write_);
            Defer d = w->d_;
            delete w;
            d->resolve(status);
        });
    });
}

}
