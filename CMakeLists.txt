# cmake -DBOOST_ROOT=path_to_boost_include -DBOOST_LIBRARYDIR=path_to_boost_lib -DCMAKE_BUILD_TYPE=Release|Debug .


cmake_minimum_required(VERSION 3.5)

# make Release as default
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    endif()
endif()

set (CMAKE_CXX_STANDARD 11)

project(promise)

# build shared option
option(PROMISE_BUILD_SHARED "Build shared library" OFF)

set(my_headers
    include/promise.hpp
    include/any.hpp
    include/add_ons.hpp
    include/call_traits.hpp
)

set(my_sources
    src/promise.cpp
    Readme.md
)

include_directories(include .)

if(PROMISE_BUILD_SHARED OR BUILD_SHARED_LIBS)
    add_definitions(-DPROMISE_BUILD_SHARED)
    add_library(promise SHARED ${my_sources} ${my_headers})
else()
    add_library(promise STATIC ${my_sources} ${my_headers})
endif()


add_executable(test0 ${my_headers} example/test0.cpp)
target_link_libraries(test0 PRIVATE promise)

add_executable(simple_timer ${my_headers} example/simple_timer.cpp)
target_link_libraries(simple_timer PRIVATE promise)

add_executable(simple_benchmark_test ${my_headers} example/simple_benchmark_test.cpp)
target_link_libraries(simple_benchmark_test PRIVATE promise)

add_executable(chain_defer_test ${my_headers} example/chain_defer_test.cpp)
target_link_libraries(chain_defer_test PRIVATE promise)


find_package(Boost)
if(NOT Boost_FOUND)
    message(WARNING "Boost not found, so asio projects will not be compiled")
else()

    include_directories(${Boost_INCLUDE_DIRS})
    if (UNIX)
        link_libraries(pthread)
    endif (UNIX)

    add_compile_definitions(BOOST_ALL_NO_LIB)
  
    include_directories(. ./add_ons/asio)
    
    add_executable(asio_benchmark_test ${my_headers} example/asio_benchmark_test.cpp)
    target_link_libraries(asio_benchmark_test PRIVATE promise)

    add_executable(asio_timer ${my_headers} example/asio_timer.cpp)
    target_link_libraries(asio_timer PRIVATE promise)

    add_executable(asio_http_client ${my_headers} example/asio_http_client.cpp)
    target_link_libraries(asio_http_client PRIVATE promise)

    add_executable(asio_http_server ${my_headers} example/asio_http_server.cpp)
    target_link_libraries(asio_http_server PRIVATE promise)
endif()


find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets)
if(NOT QT_DIR)
    message(WARNING "QT not found, so project qt_timer will not be compiled")
else()
    if(PROMISE_BUILD_SHARED OR BUILD_SHARED_LIBS)
        add_library(promise_qt SHARED ./add_ons/qt/promise_qt.cpp ${my_headers})
    else()
        add_library(promise_qt STATIC ./add_ons/qt/promise_qt.cpp ${my_headers})
    endif()
    target_link_libraries(promise_qt PRIVATE promise Qt${QT_VERSION_MAJOR}::Widgets)
    
    add_subdirectory(./example/qt_timer)
endif()

if(MSVC)
    add_subdirectory(./example/mfc_timer)
endif()
