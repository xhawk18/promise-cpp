#include "timer.hpp"
#include <boost/asio/steady_timer.hpp>
#include <chrono>

namespace promise {

Defer yield(boost::asio::io_service &io){
    auto defer = newPromise([&io](Defer d) {
        boost::asio::defer(io, [d]() {
            d.resolve();
        });

    });

    pm_assert(defer->next_.operator->() == nullptr);
    return defer;
}

Defer delay(boost::asio::io_service &io, uint64_t time_ms) {
    return newPromise([time_ms, &io](Defer &d) {
        boost::asio::steady_timer *timer =
            pm_new<boost::asio::steady_timer>(io, std::chrono::milliseconds(time_ms));
        d->any_ = timer;
        timer->async_wait([d](const boost::system::error_code& error_code) {
            if (!d->any_.empty()) {
                boost::asio::steady_timer *timer = any_cast<boost::asio::steady_timer *>(d->any_);
                d->any_.clear();
                pm_delete(timer);
                d.resolve();
            }
        });
    });
}

void cancelDelay(Defer d) {
    d = d.find_pending();
    if (d.operator->()) {
        if (!d->any_.empty()) {
            boost::asio::steady_timer *timer = any_cast<boost::asio::steady_timer *>(d->any_);
            d->any_.clear();
            timer->cancel();
            pm_delete(timer);
        }
        d.reject();
    }
}

Defer setTimeout(boost::asio::io_service &io,
                 const std::function<void(bool)> &func,
                 uint64_t time_ms) {
    return delay(io, time_ms).then([func]() {
        func(false);
    }, [func]() {
        func(true);
    });
}

void clearTimeout(Defer d) {
    cancelDelay(d);
}

}
