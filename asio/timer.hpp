#pragma once
#ifndef INC_TIMER_HPP_
#define INC_TIMER_HPP_

#include "promise.hpp"
#include <boost/asio.hpp>

namespace promise {

Defer yield(boost::asio::io_service &io);
Defer delay(boost::asio::io_service &io, uint64_t time_ms);
void cancelDelay(Defer d);

Defer setTimeout(boost::asio::io_service &io,
                 const std::function<void(bool /*cancelled*/)> &func,
                 uint64_t time_ms);
void clearTimeout(Defer d);



}
#endif
