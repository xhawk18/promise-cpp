#ifndef INC_BUFFER_HPP_
#define INC_BUFFER_HPP_

#include <stdint.h>
#include <vector>
#include <memory>

struct Buffer {
    typedef std::vector<char> inner_type;
    std::shared_ptr<inner_type> inner_data_;

    Buffer(size_t size)
        : inner_data_(new inner_type(size)) {
    }
    virtual ~Buffer() {
    }

    char *operator&() {
        return inner_data_->data();
    }

    size_t length() const {
        return inner_data_->size();
    }
};


#endif
