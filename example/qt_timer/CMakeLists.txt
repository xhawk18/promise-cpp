cmake_minimum_required(VERSION 3.5)

project(qt_timer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check https://doc.qt.io/qt/deployment-android.html for more information.
# They need to be set before the find_package( ...) calls below.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ../../include/promise-cpp/promise.hpp
    ../../include/promise-cpp/promise_inl.hpp
    ../../include/promise-cpp/any.hpp
    ../../include/promise-cpp/add_ons.hpp
    ../../include/promise-cpp/call_traits.hpp
    ../../add_ons/qt/promise_qt.hpp
)

if(${QT_VERSION_MAJOR} GREATER 5)
    qt_add_executable(qt_timer
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(qt_timer SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(qt_timer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(qt_timer PRIVATE promise promise_qt Qt${QT_VERSION_MAJOR}::Widgets)
#target_include_directories(qt_timer PRIVATE "../..")

# Set vc debugger PATH
# Supported by cmake 3.13 or higher, ignored by lower version.
string(REGEX REPLACE "[\\/]lib[\\/]cmake[\\/].*$" "/bin" MY_QT_BINARY_PATH ${QT_DIR})
file(TO_NATIVE_PATH "${MY_QT_BINARY_PATH}" MY_QT_BINARY_PATH)
message("PATH=$(PATH);${MY_QT_BINARY_PATH}")

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(PATH);${MY_QT_BINARY_PATH}")
