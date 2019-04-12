#-------------------------------------------------
#
# Project created by QtCreator 2018-05-28T21:07:53
#
#-------------------------------------------------

QT       -= gui core

LLVM = llvm-config

TARGET = Tiny-Tiger
TEMPLATE = app

INCLUDEPATH += $$system($$LLVM --includedir)

QMAKE_CXXFLAGS += $(shell $$LLVM --cxxflags)

LIBS += $(shell $$LLVM --ldflags --system-libs --libs all)

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
    src/AST/ast.cpp \
    src/codegen/codegen.cpp \
    src/utils/symboltable.cpp \
    src/utils/runtime.cpp \
    src/utils/codegencontext.cpp

HEADERS += \
    src/AST/ast.h \
    src/utils/symboltable.h \
    src/utils/codegencontext.h

OTHER_FILES += \
    src/tiger.l \
    src/tiger.y

RESOURCES += \
    res/res.qrc

DISTFILES += \
    res/icon1.png \
    res/icon2.png \
    res/icon3.png \
    res/icon4.png \
    res/icon5.png \
    res/icon6.png \
    res/icon7.png \
    res/icon8.png \
    res/icon9.png \
    res/icon10.png
