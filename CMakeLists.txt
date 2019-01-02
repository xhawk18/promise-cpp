# cmake -DBOOST_ROOT=path_to_boost_include -DBOOST_LIBRARYDIR=path_to_boost_lib -DCMAKE_BUILD_TYPE=Release|Debug .


cmake_minimum_required(VERSION 2.8)

# make Release as default
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  endif()
endif()


macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)


project(promise)
set(PROJECT_VERSION 1.0.0)
use_cxx11()
aux_source_directory(. DIR_SRCS)
include_directories(.)
add_executable(test0 test/test0.cpp)
add_executable(simple_timer test/simple_timer.cpp)
add_executable(simple_benchmark_test test/simple_benchmark_test.cpp)
add_executable(chain_defer_test test/chain_defer_test.cpp)
enable_testing()
add_test(chain_defer_test chain_defer_test)

install(FILES promise.hpp DESTINATION include)
file(GLOB PROMISE_HEADERS promise/*.hpp)
install(FILES ${PROMISE_HEADERS} DESTINATION include/promise)
configure_file(build/promise-cpp.pc.in ${CMAKE_CURRENT_BINARY_DIR}/promise-cpp.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/promise-cpp.pc DESTINATION lib/pkgconfig)
unset(PROJECT_VERSION)


project(asio)
#set(Boost_USE_STATIC_LIBS        ON) # only find static libs
#set(Boost_USE_MULTITHREADED      ON)
#set(Boost_USE_STATIC_RUNTIME     ON)
find_package(Boost 1.66.0 REQUIRED COMPONENTS date_time system)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_libraries(${Boost_LIBRARIES})
endif()

if (UNIX)
  link_libraries(pthread)
endif (UNIX)

use_cxx11()
aux_source_directory(. DIR_SRCS)
include_directories(. ./asio)
add_executable(asio_benchmark_test test/asio_benchmark_test.cpp)
add_executable(asio_timer test/asio_timer.cpp)
add_executable(asio_http_client test/asio_http_client.cpp)
add_executable(asio_http_server test/asio_http_server.cpp)


