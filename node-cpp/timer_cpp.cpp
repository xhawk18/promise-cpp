#include "uv.h"
#include "uv_defer.hpp"
#include "timer.hpp"

namespace promise {

using UvTimer = UvDefer<uv_timer_t>;

Defer yield(){
    return setTimeout(0);
}

Defer delay(uint64_t time_ms) {
    return setTimeout(time_ms);
}

Defer setTimeout(uint64_t time_ms) {
    return newPromise([time_ms](Defer &d) {
        UvTimer *handler = pm_new<UvTimer>(d);
        d->any_ = handler;

        uv_loop_t *loop = uv_default_loop();
        uv_timer_t *timer = static_cast<uv_timer_t *>(handler);
        uv_timer_init(loop, timer);
        uv_timer_start(timer, [](uv_timer_t* timer) {
            UvTimer *handler = static_cast<UvTimer *>(timer);
            Defer d = handler->d_;
            pm_delete(handler);
            d.resolve();
        }, time_ms, 0);
    });
}

void clearTimeout(Defer d) {
    d = d.find_pending();
    if (d.operator->()) {
        UvTimer *handler = any_cast<UvTimer *>(d->any_);
        if (handler != nullptr) {
            uv_timer_t *timer = static_cast<uv_timer_t *>(handler);
            uv_timer_stop(timer);
            pm_delete(handler);
        }
        d.reject();
    }
}

}
