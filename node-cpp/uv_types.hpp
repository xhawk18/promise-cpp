#ifndef INC_UV_TYPES_HPP_
#define INC_UV_TYPES_HPP_

#include "uv.h"
#include <memory>

template<class P, size_t offset, class M>
inline P* uv_container_of(M* ptr) {
    return reinterpret_cast<P*>(reinterpret_cast<char*>(ptr) - offset);
}

template<class M, size_t offset, class P>
inline M* uv_element_of(P* ptr) {
    return reinterpret_cast<M*>(reinterpret_cast<char*>(ptr) + offset);
}



template <class T>
struct UvMakeShared {
    typedef T inner_type;
    std::shared_ptr<inner_type> inner_data_;

    UvMakeShared()
        : inner_data_(new inner_type()) {
    }
    virtual ~UvMakeShared() {
    }

    inner_type *operator&() {
        return inner_data_.get();
    }
};




#if 0
/* Handle types. */
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_handle_s uv_handle_t;
typedef struct uv_stream_s uv_stream_t;
typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_udp_s uv_udp_t;
typedef struct uv_pipe_s uv_pipe_t;
typedef struct uv_tty_s uv_tty_t;
typedef struct uv_poll_s uv_poll_t;
typedef struct uv_timer_s uv_timer_t;
typedef struct uv_prepare_s uv_prepare_t;
typedef struct uv_check_s uv_check_t;
typedef struct uv_idle_s uv_idle_t;
typedef struct uv_async_s uv_async_t;
typedef struct uv_process_s uv_process_t;
typedef struct uv_fs_event_s uv_fs_event_t;
typedef struct uv_fs_poll_s uv_fs_poll_t;
typedef struct uv_signal_s uv_signal_t;

/* Request types. */
typedef struct uv_req_s uv_req_t;
typedef struct uv_getaddrinfo_s uv_getaddrinfo_t;
typedef struct uv_getnameinfo_s uv_getnameinfo_t;
typedef struct uv_shutdown_s uv_shutdown_t;
#endif
typedef struct uv_write_s uv_write_t;

using UvWrite = UvMakeShared<uv_write_t>;

#if 0
typedef struct uv_connect_s uv_connect_t;
typedef struct uv_udp_send_s uv_udp_send_t;
typedef struct uv_fs_s uv_fs_t;
typedef struct uv_work_s uv_work_t;

/* None of the above. */
typedef struct uv_cpu_info_s uv_cpu_info_t;
typedef struct uv_interface_address_s uv_interface_address_t;
typedef struct uv_dirent_s uv_dirent_t;
typedef struct uv_passwd_s uv_passwd_t;
#endif


#endif
