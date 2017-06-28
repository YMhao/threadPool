TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lpthread

SOURCES += main.cpp \
    threadPool.cpp \
    thread.cpp

HEADERS += \
    threadPool.h \
    thread.h \
    task.h
