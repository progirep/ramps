# QMake Build file
QMAKE_CC = gcc
QMAKE_LINK_C = gcc
QMAKE_CXX = g++
QMAKE_LINK = g++
DEFINES += NDEBUG
CFLAGS += -g -fpermissive

QMAKE_CFLAGS_RELEASE += -g
QMAKE_CXXFLAGS_RELEASE += -g -std=gnu++0x
QMAKE_CFLAGS_DEBUG += -g -Wall -Wextra
QMAKE_CXXFLAGS_DEBUG += -g -std=gnu++0x -Wall -Wextra

TEMPLATE = app console
CONFIG += debug
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += mdp.hpp

SOURCES += main.cpp mdp.cpp

TARGET = ramps
INCLUDEPATH = .

LIBS += 

PKGCONFIG += 
QT -= gui core
