QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    ../../src/promise.cpp \
    ../../add_ons/qt/promise_qt.cpp

HEADERS += \
    mainwindow.h \
    ../../include/promise-cpp/promise.hpp
    ../../include/promise-cpp/promise_inl.hpp
    ../../add_ons/qt/promise_qt.hpp
    ../../add_ons/qt/promise_qt_inl.hpp

FORMS += \
    mainwindow.ui

INCLUDEPATH += \
    ../../include \
    ../../

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
