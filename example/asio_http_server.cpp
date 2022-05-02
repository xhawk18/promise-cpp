// This code is forked from
//   boost_1_67_0/libs/beast/example/http/server/coro/http_server_coro.cpp
// and modified by xhawk18 to use promise-cpp for better async control.
// Copyright (c) 2018, xhawk18
// at gmail.com

//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, asynchronous with promise-cpp
//
//------------------------------------------------------------------------------

#include <boost/version.hpp>
#if BOOST_VERSION < 106600
#   error "This program need boost 1.66.0 or higher!"
#endif
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>


#include <promise-cpp/add_ons/asio/io.hpp>

using namespace promise;

namespace asio        = boost::asio;
namespace ip          = boost::asio::ip;
using     socket_base = boost::asio::socket_base;
using     tcp         = boost::asio::ip::tcp;
namespace beast       = boost::beast;
namespace http        = boost::beast::http;

struct Session {
    bool close_;
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    std::string doc_root_;
    http::request<http::string_body> req_;

    explicit Session(
        tcp::socket &socket,
        std::string const& doc_root
        )
        : close_(false)
        , socket_(std::move(socket))
        , doc_root_(doc_root) {
        std::cout << "new session" << std::endl;
    }

    ~Session() {
        std::cout << "delete session" << std::endl;
    }
};

// Promisified functions
Promise async_accept(tcp::acceptor &acceptor) {
    return newPromise([&](Defer &defer) {
        // Look up the domain name
        auto socket = std::make_shared<tcp::socket>(static_cast<asio::io_context &>(acceptor.get_executor().context()));
        acceptor.async_accept(*socket,
            [=](boost::system::error_code err) {
            setPromise(defer, err, "resolve", socket);
        });
    });
}


// Return a reasonable mime type based on the extension of a file.
beast::string_view
mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string
path_cat(
    beast::string_view base,
    beast::string_view path)
{
    if(base.empty())
        return path.to_string();
    std::string result = base.to_string();
#if BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template<class Send>
Promise
handle_request(
    std::shared_ptr<Session> session,
    Send&& send)
{
    // Returns a bad request response
    auto const bad_request =
    [session](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, session->req_.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(session->req_.keep_alive());
        res.body() = why.to_string();
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [session](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, session->req_.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(session->req_.keep_alive());
        res.body() = "The resource '" + target.to_string() + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [session](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, session->req_.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(session->req_.keep_alive());
        res.body() = "An error occurred: '" + what.to_string() + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if(session->req_.method() != http::verb::get &&
        session->req_.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // Request path must be absolute and not contain "..".
    if(session->req_.target().empty() ||
        session->req_.target()[0] != '/' ||
        session->req_.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // Build the path to the requested file
    std::string path = path_cat(session->doc_root_, session->req_.target());
    if(session->req_.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == boost::system::errc::no_such_file_or_directory)
        return send(not_found(session->req_.target()));

    // Handle an unknown error
    if(ec)
        return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if(session->req_.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, session->req_.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(session->req_.keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, session->req_.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(session->req_.keep_alive());
    return send(std::move(res));
}

//------------------------------------------------------------------------------

// Report a failure
void
fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// This is the C++11 equivalent of a generic lambda.
// The function object is used to send an HTTP message.
template<class Stream>
struct send_lambda
{
    Stream& stream_;
    bool& close_;

    explicit
    send_lambda(
        Stream& stream,
        bool& close)
        : stream_(stream)
        , close_(close)
    {
    }

    template<bool isRequest, class Body, class Fields>
    Promise
    operator()(http::message<isRequest, Body, Fields>&& msg) const
    {
        // Determine if we should close the connection after
        close_ = msg.need_eof();
        // The lifetime of the message has to extend
        // for the duration of the async operation so
        // we use a shared_ptr to manage it.
        auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));
        return async_write(stream_, *sp).finally([sp]() {
            //sp will be deleted util finally() called
        });
    }
};

// Handles an HTTP server connection
void
do_session(
    std::shared_ptr<Session> session)
{
    doWhile([=](DeferLoop &loop){
        std::cout << "read new http request ... " << std::endl;
        //<1> Read a request
        session->req_ = {};
        async_read(session->socket_, session->buffer_, session->req_)

        .then([=]() {
            //<2> Send the response
            // This lambda is used to send messages
            send_lambda<tcp::socket> lambda{ session->socket_, session->close_ };
            return handle_request(session, lambda);

        }).then([]() {
            //<3> success, return default error_code
            return boost::system::error_code();
        }, [](const boost::system::error_code err) {
            //<3> failed, return the error_code
            return err;

        }).then([=](boost::system::error_code &err) {
            //<4> Keep-alive or close the connection.
            if (!err && !session->close_) {
                loop.doContinue();//continue doWhile ...
            }
            else {
                std::cout << "shutdown..." << std::endl;
                session->socket_.shutdown(tcp::socket::shutdown_send, err);
                loop.doBreak(); //break from doWhile
            }
        });
    });
}

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
void
do_listen(
    asio::io_context& ioc,
    tcp::endpoint endpoint,
    std::string const& doc_root)
{
    boost::system::error_code ec;

    // Open the acceptor
    auto acceptor = std::make_shared<tcp::acceptor>(ioc);
    acceptor->open(endpoint.protocol(), ec);
    if(ec)
        return fail(ec, "open");

    // Allow address reuse
    acceptor->set_option(socket_base::reuse_address(true));
    if(ec)
        return fail(ec, "set_option");

    // Bind to the server address
    acceptor->bind(endpoint, ec);
    if(ec)
        return fail(ec, "bind");

    // Start listening for connections
    acceptor->listen(socket_base::max_listen_connections, ec);
    if(ec)
        return fail(ec, "listen");

    auto doc_root_ = std::make_shared<std::string>(doc_root);
    doWhile([acceptor, doc_root_](DeferLoop &loop){
        async_accept(*acceptor).then([=](std::shared_ptr<tcp::socket> socket) {
            std::cout << "accepted" << std::endl;
            auto session = std::make_shared<Session>(*socket, *doc_root_);
            do_session(session);
        }).fail([](const boost::system::error_code err) {
        }).then(loop);
    });
}

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 5)
    {
        std::cerr <<
            "Usage: http-server-coro <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http-server-coro 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address = ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    std::string const doc_root = argv[3];
    auto const threads = std::max<int>(1, std::atoi(argv[4]));

    // The io_context is required for all I/O
    asio::io_context ioc{threads};

    // Spawn a listening port
    do_listen(ioc,
              tcp::endpoint{ address, port },
              doc_root);

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
}
