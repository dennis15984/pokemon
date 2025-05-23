QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    laboratoryscene.cpp \
    main.cpp \
    mainwindow.cpp \
    game.cpp \
    pokemon.cpp \
    scene.cpp \
    titlescene.cpp \
    townscene.cpp \
    grasslandscene.cpp

HEADERS += \
    grasslandscene.h \
    laboratoryscene.h \
    mainwindow.h \
    game.h \
    pokemon.h \
    scene.h \
    titlescene.h \
    townscene.h
    grasslandscene.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
term_project_zh_TW.ts

CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    data.qrc
