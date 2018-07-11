#pragma once
#ifndef INC_TIMER_HPP_
#define INC_TIMER_HPP_

#include "promise.hpp"
#include <boost/asio.hpp>

namespace promise {

Defer yield(boost::asio::io_service &io);
Defer delay(boost::asio::io_service &io, uint64_t time_ms);
Defer setTimeout(boost::asio::io_service &io, uint64_t time_ms);
void clearTimeout(Defer d);



}
#endif
