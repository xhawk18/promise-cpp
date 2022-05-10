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
option(PROMISE_BUILD_EXAMPLES "Build examples" ON)

set(my_headers
    include/promise-cpp/promise.hpp
    include/promise-cpp/promise_inl.hpp
    include/promise-cpp/any.hpp
    include/promise-cpp/add_ons.hpp
    include/promise-cpp/call_traits.hpp
)

set(my_sources
    src/promise.cpp
    Readme.md
)

if(PROMISE_BUILD_SHARED OR BUILD_SHARED_LIBS)
    add_definitions(-DPROMISE_BUILD_SHARED)
    add_library(promise SHARED ${my_sources} ${my_headers})
else()
    add_library(promise STATIC ${my_sources} ${my_headers})
endif()

target_include_directories(promise PUBLIC include .)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets)
if(NOT QT_FOUND)
    message(WARNING "QT not found, so project qt_timer will not be compiled")
else()
    if(PROMISE_BUILD_SHARED OR BUILD_SHARED_LIBS)
        add_library(promise_qt SHARED ./add_ons/qt/promise_qt.cpp ${my_headers})
    else()
        add_library(promise_qt STATIC ./add_ons/qt/promise_qt.cpp ${my_headers})
    endif()
    target_link_libraries(promise_qt PRIVATE promise Qt${QT_VERSION_MAJOR}::Widgets)
endif()

if(PROMISE_BUILD_EXAMPLES) 
    add_executable(test0 ${my_headers} example/test0.cpp)
    target_link_libraries(test0 PRIVATE promise)


    find_package(Threads)
    if(Threads_FOUND)
        add_executable(simple_timer ${my_headers} example/simple_timer.cpp)
        target_link_libraries(simple_timer PRIVATE promise Threads::Threads)

        add_executable(simple_benchmark_test ${my_headers} example/simple_benchmark_test.cpp)
        target_link_libraries(simple_benchmark_test PRIVATE promise Threads::Threads)
    
        add_executable(multithread_test ${my_headers} example/multithread_test.cpp)
        target_link_libraries(multithread_test PRIVATE promise Threads::Threads)
    endif()

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

        include_directories(. ./add_ons/asio)
        
        add_executable(asio_benchmark_test ${my_headers} example/asio_benchmark_test.cpp)
        target_compile_definitions(asio_benchmark_test PRIVATE BOOST_ALL_NO_LIB)  
        target_link_libraries(asio_benchmark_test PRIVATE promise)

        add_executable(asio_timer ${my_headers} example/asio_timer.cpp)
        target_compile_definitions(asio_timer PRIVATE BOOST_ALL_NO_LIB)  
        target_link_libraries(asio_timer PRIVATE promise)

        add_executable(asio_http_client ${my_headers} example/asio_http_client.cpp)
        target_compile_definitions(asio_http_client PRIVATE BOOST_ALL_NO_LIB)  
        target_link_libraries(asio_http_client PRIVATE promise)

        add_executable(asio_http_server ${my_headers} example/asio_http_server.cpp)
        target_compile_definitions(asio_http_server PRIVATE BOOST_ALL_NO_LIB)  
        target_link_libraries(asio_http_server PRIVATE promise)
    endif()

    if(QT_FOUND)
        add_subdirectory(./example/qt_timer)
    endif()

    find_package(MFC)
    if(NOT MFC_FOUND)
        message(WARNING "MFC not found, so project mfc_timer will not be compiled")
    else()
        add_subdirectory(./example/mfc_timer)
    endif()
endif()
