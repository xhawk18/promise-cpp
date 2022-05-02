// This code is forked from
//   boost_1_67_0/libs/beast/example/http/client/coro/http_client_coro.cpp
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
// Example: HTTP client, asynchronous with promise-cpp
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
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <regex>
#include <promise-cpp/add_ons/asio/io.hpp>

using namespace promise;
namespace asio   = boost::asio;
using     tcp    = boost::asio::ip::tcp;
namespace http   = boost::beast::http;
namespace beast  = boost::beast;

//------------------------------------------------------------------------------

//From https://stackoverflow.com/a/11044337
struct Uri
{
public:
    std::string QueryString, Path, Protocol, Host, Port;

    static Uri Parse(const std::string &uri)
    {
        Uri result;

        typedef std::string::const_iterator iterator_t;

        if (uri.length() == 0)
            return result;

        iterator_t uriEnd = uri.end();

        // get query start
        iterator_t queryStart = std::find(uri.begin(), uriEnd, '?');

        // protocol
        iterator_t protocolStart = uri.begin();
        iterator_t protocolEnd = std::find(protocolStart, uriEnd, ':');            //"://");

        if (protocolEnd != uriEnd)
        {
            std::string prot = &*(protocolEnd);
            if ((prot.length() > 3) && (prot.substr(0, 3) == "://"))
            {
                result.Protocol = std::string(protocolStart, protocolEnd);
                protocolEnd += 3;   //      ://
            }
            else
                protocolEnd = uri.begin();  // no protocol
        }
        else
            protocolEnd = uri.begin();  // no protocol

        // host
        iterator_t hostStart = protocolEnd;
        iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart

        iterator_t hostEnd = std::find(protocolEnd, 
            (pathStart != uriEnd) ? pathStart : queryStart,
            L':');  // check for port

        result.Host = std::string(hostStart, hostEnd);

        // port
        if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == ':'))  // we have a port
        {
            hostEnd++;
            iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
            result.Port = std::string(hostEnd, portEnd);
        }

        // path
        if (pathStart != uriEnd)
            result.Path = std::string(pathStart, queryStart);

        // query
        if (queryStart != uriEnd)
            result.QueryString = std::string(queryStart, uri.end());

        return result;

    }   // Parse
};  // uri

// Performs an HTTP GET and prints the response
Promise do_session(
    std::string const& host,
    std::string const& port,
    std::string const& target,
    int version,
    asio::io_context& ioc) {

    struct Session {
        tcp::resolver resolver_;
        tcp::socket socket_;
        beast::flat_buffer buffer_;
        http::request<http::empty_body> req_;
        http::response<http::string_body> res_;

        explicit Session(asio::io_context& ioc)
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
    return async_resolve(session->resolver_, host, port)

    .then([=](tcp::resolver::results_type &results) {
        //<2> Connect to the host
        return async_connect(session->socket_, results);

    }).then([=]() {
        //<3> Write the request
        return async_write(session->socket_, session->req_);

    }).then([=](size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        //<4> Read the response
        return async_read(session->socket_, session->buffer_, session->res_);

    }).then([=](size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        //<5> Write the message to standard out
        std::cout << session->res_ << std::endl;

    }).then([]() {
        //<6> success, return default error_code
        return boost::system::error_code();
    }, [](const boost::system::error_code err) {
        //<6> failed, return the error_code
        return err;

    }).then([=](boost::system::error_code &err) {
        //<7> Gracefully close the socket
        std::cout << "shutdown..." << std::endl;
        session->socket_.shutdown(tcp::socket::shutdown_both, err);
    });
}


Promise download(asio::io_context &ioc, const std::string &url) {
    const Uri &uri = Uri::Parse(url);
    if (uri.Protocol != "http") {
        std::cerr << "Support http protocol only!\n";
        return promise::reject();
    }

    std::string port = (uri.Port.empty() ? "80" : uri.Port);
    std::string target = uri.Path + uri.QueryString;

    std::cout << target << "\n";
    std::cout << port << "\n";
    std::cout << uri.Host << "\n";
 
    int http_version = 10;
    return do_session(uri.Host, port, target, http_version, ioc);

}

//------------------------------------------------------------------------------

int main(int argc, char** argv) {
    // The io_context is required for all I/O
    asio::io_context ioc;

    // Launch the asynchronous operation
    download(ioc, "http://www.163.com/")
    .then([&]() {
        return download(ioc, "http://baidu.com/");
    }).then([&]() {
        return download(ioc, "http://qq.com");
    }).then([&]() {
        return download(ioc, "http://github.com/xhawk18");
    });

    // Run the I/O service. The call will return when
    // the get operation is complete.
    ioc.run();

    return EXIT_SUCCESS;
}
