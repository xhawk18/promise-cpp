
#include "uv.h"
#include "timer.hpp"

namespace promise {

struct UvTimer : public uv_timer_t {
    Defer d_;

    UvTimer(const Defer &d)
        : d_(d) {}

    static void onTimer(uv_timer_t* timer) {
        UvTimer *handler = static_cast<UvTimer *>(timer);
        Defer d = handler->d_;
        delete handler;
        d.resolve();
    }

    void* operator new(size_t size){
        return allocator<UvTimer>::obtain(size);
    }

    void operator delete(void *ptr) {
        allocator<UvTimer>::release(ptr);
    }
};

void setTimeout(Defer &d, uint64_t time_ms) {
    UvTimer *handler = new UvTimer(d);
    d->any_ = handler;

    UvTimer *handler2 = any_cast<UvTimer *>(d->any_);

    uv_loop_t *loop = uv_default_loop();
    uv_timer_t *timer = static_cast<uv_timer_t *>(handler2);
    uv_timer_init(loop, timer);
    uv_timer_start(timer, &UvTimer::onTimer, time_ms, 0);
}

Defer delay(uint64_t time_ms) {
    return setTimeout(time_ms);
}

Defer setTimeout(uint64_t time_ms) {
    return newPromise([time_ms](Defer d) {
        setTimeout(d, time_ms);
    });
}

void clearTimeout(Defer d) {
    d = d.find_pending();
    if (d.operator->()) {
        UvTimer *handler = any_cast<UvTimer *>(d->any_);
        if (handler != nullptr) {
            uv_timer_t *timer = static_cast<uv_timer_t *>(handler);
            uv_timer_stop(timer);
            delete handler;
        }
        d.reject();
    }
}

}
