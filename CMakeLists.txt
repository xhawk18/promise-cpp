# cmake -DBOOST_ROOT=path_to_boost_include -DBOOST_LIBRARYDIR=path_to_boost_lib -DCMAKE_BUILD_TYPE=Release|Debug .


cmake_minimum_required(VERSION 2.8)


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
use_cxx11()
aux_source_directory(. DIR_SRCS)
include_directories(.)
add_executable(test0 test/test0.cpp)
add_executable(test_timer_simple test/test_timer_simple.cpp)


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
add_executable(test_tasks test/test_tasks.cpp)
add_executable(test_timer test/test_timer.cpp)
add_executable(test_http_client test/test_http_client.cpp)
add_executable(test_http_server test/test_http_server.cpp)


