TARGET = Sample

QT = core gui
CONFIG += c++17
greaterThan(QT_MAJOR_VERSION, 6): QT += widgets
SOURCES += \
    main.cpp

HEADERS += \
    MyMainWindow.h