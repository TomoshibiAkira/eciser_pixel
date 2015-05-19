#-------------------------------------------------
#
# Project created by QtCreator 2015-05-07T00:31:53
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = eciser_pixel
TEMPLATE = app

LIBS += -LD:/PROGRAM/ANN/MS_Win32/dll/Release -lANN

SOURCES += main.cpp\
        mainwindow.cpp \
    rasterhandler.cpp \
    about.cpp

HEADERS  += mainwindow.h \
    rasterhandler.h \
    about.h

FORMS    += mainwindow.ui \
    about.ui

RESOURCES += \
    icon.qrc
