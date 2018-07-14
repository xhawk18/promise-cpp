// This code is forked from
//   boost_1_67_0/libs/beast/example/http/client/async/http_client_async.cpp
// and modified by xhawk18 to use promise-cpp for better async control.
// Copyright (c) 2018, xhawk18
// at gmail.com

// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP client, asynchronous
//
//------------------------------------------------------------------------------

#include <boost/version.hpp>
#if BOOST_VERSION < 106600
#   error "This program need boost 1.66.0 or higher!"
#endif
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "promise.hpp"

using namespace promise;

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

                                        //------------------------------------------------------------------------------

template<typename RESULT>
void setPromise(Defer d,
                boost::system::error_code ec,
                const char *errorString,
                const RESULT &result) {
    if (ec) {
        std::cerr << errorString << ": " << ec.message() << "\n";
        d.reject(ec);
    }
    else
        d.resolve(result);
}

// Promisified functions
Defer async_resolve(tcp::resolver &resolver, const std::string &host, const std::string &port) {
    return newPromise([&](Defer d) {
        // Look up the domain name
        resolver.async_resolve(
            host,
            port,
            [d](boost::system::error_code ec,
                tcp::resolver::results_type results) {
                setPromise(d, ec, "resolve", results);
        });
    });
}

Defer async_connect(tcp::socket &socket, const tcp::resolver::results_type &results) {
    return newPromise([&](Defer d) {
        // Make the connection on the IP address we get from a lookup
        boost::asio::async_connect(
            socket,
            results.begin(),
            results.end(),
            [d](boost::system::error_code ec, tcp::resolver::iterator i) {
                setPromise(d, ec, "connect", i);
        });
    });
}

Defer async_write(tcp::socket &socket, http::request<http::empty_body> &req) {
    return newPromise([&](Defer d) {
        //write
        // Send the HTTP request to the remote host
        http::async_write(socket, req,
            [d](boost::system::error_code ec,
                std::size_t bytes_transferred) {
                setPromise(d, ec, "write", bytes_transferred);
        });
    });
}

Defer async_read(tcp::socket &socket,
                 boost::beast::flat_buffer &buffer,
                 http::response<http::string_body> &res) {
    //read
    return newPromise([&](Defer d) {
        http::async_read(socket, buffer, res,
            [d](boost::system::error_code ec,
                std::size_t bytes_transferred) {
                setPromise(d, ec, "read", bytes_transferred);
        });
    });
}

// Performs an HTTP GET and prints the response
void do_session(
    std::string const& host,
    std::string const& port,
    std::string const& target,
    int version,
    boost::asio::io_context& ioc) {

    struct Session {
        tcp::resolver resolver_;
        tcp::socket socket_;
        boost::beast::flat_buffer buffer_;
        http::request<http::empty_body> req_;
        http::response<http::string_body> res_;

        explicit Session(boost::asio::io_context& ioc)
            : resolver_(ioc)
            , socket_(ioc) {
        }
    };

    // (Must persist in io_context ioc)
    auto session = std::make_shared<Session>(ioc);

    // Set up an HTTP GET request message
    session->req_.version(version);
    session->req_.method(http::verb::get);
    session->req_.target(target);
    session->req_.set(http::field::host, host);
    session->req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    //<1> Resolve the host
    async_resolve(session->resolver_, host, port)

    .then([=](tcp::resolver::results_type &results) {
        //<2> Connect to the host
        return async_connect(session->socket_, results);

    }).then([=]() {
        //<3> Write the request
        return async_write(session->socket_, session->req_);

    }).then([=](std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        //<4> Read the response
        return async_read(session->socket_, session->buffer_, session->res_);

    }).then([=](std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        //<5> Write the message to standard out
        std::cout << session->res_ << std::endl;

    }).then([]() {
        //<6> success, return default error_code
        return boost::system::error_code();
    }, [](const boost::system::error_code ec) {
        //<6> failed, return the error_code
        return ec;

    }).then([=](boost::system::error_code &ec) {
        //<7> Gracefully close the socket
        session->socket_.shutdown(tcp::socket::shutdown_both, ec);
    });
}

//------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    // Check command line arguments.
    if (argc != 4 && argc != 5)
    {
        std::cerr <<
            "Usage: http-client-async <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
            "Example:\n" <<
            "    http-client-async www.example.com 80 /\n" <<
            "    http-client-async www.example.com 80 / 1.0\n";
        return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];
    auto const target = argv[3];
    int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // Launch the asynchronous operation
    do_session(host, port, target, version, ioc);

    // Run the I/O service. The call will return when
    // the get operation is complete.
    ioc.run();

    return EXIT_SUCCESS;
}
