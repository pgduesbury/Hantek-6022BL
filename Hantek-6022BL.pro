#-------------------------------------------------
#
# Project created by QtCreator 2016-03-04T07:43:15
#
#-------------------------------------------------

QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Hantek-6022BL
TEMPLATE = app
INCLUDEPATH += /usr/include/libusb-1.0
LIBS += -L/usr/lib
LIBS +=-lusb-1.0
LIBS +=-lm

SOURCES += main.cpp\
        mainwindow.cpp \
    HT6022fw.c \
    HT6022.c \
    worker.cpp \
    qcustomplot.cpp \
    DSOutils.c \
    PostTrig.c

HEADERS  += mainwindow.h \
    HT6022fw.h \
    HT6022.h \
    worker.h \
    qcustomplot.h \
    DSOutils.h \
    dso.h \
    PostTrig.h

FORMS    += mainwindow.ui
