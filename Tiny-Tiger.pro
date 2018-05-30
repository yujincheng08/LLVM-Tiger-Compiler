#-------------------------------------------------
#
# Project created by QtCreator 2018-05-28T21:07:53
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Tiny-Tiger
TEMPLATE = app


LEXSOURCES = src/tiger.l
YACCSOURCES = src/tiger.y

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    src/

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/util.c \
    src/table.c \
    src/symbol.c \
    src/errormsg.c \
    src/absyn.c \
    src/types.c

HEADERS += \
    src/mainwindow.h \
    src/util.h \
    src/table.h \
    src/symbol.h \
    src/errormsg.h \
    src/absyn.h \
    src/types.h

OTHER_FILES += \
    src/tiger.l \
    src/tiger.y
