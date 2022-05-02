/*
* Copyright (c) 2016, xhawk18
* at gmail.com
*
* The MIT License (MIT)
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#pragma once
#ifndef INC_ASIO_IO_HPP_
#define INC_ASIO_IO_HPP_

#include "promise-cpp/promise.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace promise{

template<typename RESULT>
inline void setPromise(Defer defer,
    boost::system::error_code err,
    const char *errorString,
    const RESULT &result) {
    if (err) {
        std::cerr << errorString << ": " << err.message() << "\n";
        defer.reject(err);
    }
    else
        defer.resolve(result);
}

// Promisified functions
template<typename Resolver>
inline Promise async_resolve(
    Resolver &resolver,
    const std::string &host, const std::string &port) {
    return newPromise([&](Defer &defer) {
        // Look up the domain name
        resolver.async_resolve(
            host,
            port,
            [defer](boost::system::error_code err,
                typename Resolver::results_type results) {
                setPromise(defer, err, "resolve", results);
        });
    });
}

template<typename ResolverResult, typename Socket>
inline Promise async_connect(
    Socket &socket,
    const ResolverResult &results) {
    return newPromise([&](Defer &defer) {
        // Make the connection on the IP address we get from a lookup
        boost::asio::async_connect(
            socket,
            results.begin(),
            results.end(),
            [defer](boost::system::error_code err,
                typename ResolverResult::iterator i) {
                setPromise(defer, err, "connect", i);
        });
    });
}



template<typename Stream, typename Buffer, typename Content>
inline Promise async_read(Stream &stream,
    Buffer &buffer,
    Content &content) {
    //read
    return newPromise([&](Defer &defer) {
        boost::beast::http::async_read(stream, buffer, content,
            [defer](boost::system::error_code err,
                std::size_t bytes_transferred) {
                setPromise(defer, err, "read", bytes_transferred);
        });
    });
}

template<typename Stream, typename Content>
inline Promise async_write(Stream &stream, Content &content) {
    return newPromise([&](Defer &defer) {
        //write
        boost::beast::http::async_write(stream, content,
            [defer](boost::system::error_code err,
                std::size_t bytes_transferred) {
                setPromise(defer, err, "write", bytes_transferred);
        });
    });
}


}
#endif
