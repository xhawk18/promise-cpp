#include "uv.h"
#include "promise.hpp"
#include "buffer.hpp"
#include "stream.hpp"
#include "uv_types.hpp"
using namespace promise;

#if 1
  
static void tinyweb_on_connection(uv_stream_t* server, int status);  

void tinyweb_start(uv_loop_t* loop, const char* ip, int port) {  
	uv_tcp_t *server = new uv_tcp_t;

	struct sockaddr_in addr;
    uv_tcp_init(loop, server);
	uv_ip4_addr(ip, port, &addr);
    uv_tcp_bind(server, (struct sockaddr *)&addr, 0);
    uv_listen((uv_stream_t*)server, 8, tinyweb_on_connection);
}  
  
static void after_uv_close(uv_handle_t* handle) {  
    free(handle); //uv_tcp_t* client  
}  
  
static void after_uv_write(uv_write_t* w, int status) {  
    if(w->data)  
        free(w->data);  
    uv_close((uv_handle_t*)w->handle, after_uv_close); //close client  
    free(w);  
}  
  
static void write_uv_data(uv_stream_t* stream, const void* data, unsigned int len, int need_copy_data) {  
    if(data == NULL || len == 0) return;  
    if(len ==(unsigned int)-1)  
        len = strlen((const char *)data);  
  
    void* newdata  = (void*)data;  
    if(need_copy_data) {  
        newdata = malloc(len);  
        memcpy(newdata, data, len);  
    }  
  
    uv_buf_t buf = uv_buf_init((char *)newdata, len);  
    uv_write_t* w = (uv_write_t*)malloc(sizeof(uv_write_t));  
    w->data = need_copy_data ? newdata : NULL;  
    //free w and w->data in after_uv_write()  
    uv_write(w, stream, &buf, 1, after_uv_write);  
}  
  
static const char* http_respone = "HTTP/1.1 200 OK\r\n"  
    "Content-Type:text/html;charset=utf-8\r\n"  
    "Content-Length:11\r\n"  
    "\r\n"  
    "Hello world";  



static void tinyweb_on_connection(uv_stream_t* server, int status) {  

	if(status == 0) {  
        uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
        uv_tcp_init(server->loop, client);

        if (uv_accept((uv_stream_t*)server, (uv_stream_t*)client) != 0)
            return;
#if 1
        Buffer buf(333);
        cuv_read((uv_stream_t *)client, buf, buf.length()).then([buf](UvRead &r) {
            printf("%d nread = %d\n", __LINE__, r.nread_);
            return cuv_read(r, buf, buf.length());
        }).then([buf](UvRead &r) {
            printf("%d nread = %d\n", __LINE__, r.nread_);
            return cuv_read(r, buf, buf.length());
        }).then([buf](UvRead &r) {
            printf("%d nread = %d\n", __LINE__, r.nread_);
            return cuv_read(r, buf, buf.length());
        }).then([buf](UvRead &r) {
            printf("%d nread = %d\n", __LINE__, r.nread_);
            return cuv_read(r, buf, buf.length());
        }).always([](UvRead &r) {
            printf("%d nread = %d\n", __LINE__, r.nread_);
            //cuv_read_stop(r);
        });

#else
        uv_read_start((uv_stream_t *)client, [](uv_handle_t* handle,
            size_t suggested_size,
            uv_buf_t* buf) {
            printf("suggested_size = %d, buf = %p\n", suggested_size, buf);
            char *p = (char *)malloc(suggested_size);
            *buf = uv_buf_init(p, suggested_size);
        }, [](uv_stream_t* stream,
            ssize_t nread,
            const uv_buf_t* buf) {
            printf("nread = %d, buf = %p\n", nread, buf);
            if (nread < 0) {
                if (nread != UV_EOF)
                    fprintf(stderr, "read error %s\n", uv_strerror(nread));
                uv_close((uv_handle_t *)stream, NULL);
            }
            //else uv_read_stop(stream);
        });

#endif
        //write_uv_data((uv_stream_t*)client, http_respone, -1, 0);  
        //close client after uv_write, and free it in after_uv_close()  
    }  
}  


int main() {
	uv_loop_t *loop = uv_default_loop();

	tinyweb_start(loop, "127.0.0.1", 8080);
	return uv_run(loop, UV_RUN_DEFAULT);
}
#endif
