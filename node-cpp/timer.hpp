#ifndef INC_TIMER_HPP_
#define INC_TIMER_HPP_

#include "../promise.hpp"

namespace promise {

Defer delay(uint64_t time_ms);
Defer setTimeout(uint64_t time_ms);
void clearTimeout(Defer d);



}
#endif
