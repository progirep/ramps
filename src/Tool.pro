# QMake Build file
QMAKE_CC = gcc
QMAKE_LINK_C = gcc
QMAKE_CXX = g++
QMAKE_LINK = g++
DEFINES += # No NDEBUG here.
CFLAGS += -g -fpermissive

QMAKE_CFLAGS_RELEASE += -g -march=native -fopenmp
QMAKE_CXXFLAGS_RELEASE += -g -std=c++11  -march=native -fopenmp
QMAKE_CFLAGS_DEBUG += -g -Wall -Wextra  -march=native -fopenmp
QMAKE_CXXFLAGS_DEBUG += -g -std=c++11 -Wall -Wextra  -march=native -fopenmp
QMAKE_LFLAGS += -fopenmp

TEMPLATE = app console
CONFIG += release
CONFIG -= app_bundle
CONFIG -= qt

HEADERS += mdp.hpp

SOURCES += main.cpp mdp.cpp computePolicy.cpp

TARGET = ramps
INCLUDEPATH =

LIBS +=

PKGCONFIG += 
QT -= gui core
