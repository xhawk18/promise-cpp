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

set(my_source
    include/promise.hpp
    include/any.hpp
    include/add_ons.hpp
    include/call_traits.hpp
    src/promise.cpp
    Readme.md
)

include_directories(include .)

add_executable(test0 ${my_source} example/test0.cpp)
add_executable(simple_timer ${my_source} example/simple_timer.cpp)
add_executable(simple_benchmark_test ${my_source} example/simple_benchmark_test.cpp)


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
    add_executable(asio_benchmark_test ${my_source} example/asio_benchmark_test.cpp)
    add_executable(asio_timer ${my_source} example/asio_timer.cpp)
    add_executable(asio_http_client ${my_source} example/asio_http_client.cpp)
    add_executable(asio_http_server ${my_source} example/asio_http_server.cpp)
endif()


find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets)
if(NOT QT_DIR)
    message(WARNING "QT not found, so project qt_timer will not be compiled")
else()
    add_subdirectory(./example/qt_timer)
endif()

if(MSVC)
    add_subdirectory(./example/mfc_timer)
endif()
