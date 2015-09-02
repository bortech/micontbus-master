#-------------------------------------------------
#
# Project created by QtCreator 2015-08-26T11:04:20
#
#-------------------------------------------------

QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = micontbus_master
TEMPLATE = app


SOURCES += main.cpp\
    micontbuspacket.cpp \
    micontbusmaster.cpp \
    window.cpp

HEADERS  += \
    micontbuspacket.h \
    micontbusmaster.h \
    window.h
