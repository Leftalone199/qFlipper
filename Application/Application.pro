QT += quick serialport widgets

TARGET = flipartner
DESTDIR = ..

CONFIG += c++11

SOURCES += \
        main.cpp

RESOURCES += qml.qrc

TRANSLATIONS += \
    Application_en_US.ts

CONFIG += lrelease
CONFIG += embed_translations

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

unix|win32-g++ {
    LIBS += \
        -L$$OUT_PWD/../Backend/ -lBackend \
        -L$$OUT_PWD/../Dfu/ -lDfu

    PRE_TARGETDEPS += \
        $$OUT_PWD/../Backend/libBackend.a \
        $$OUT_PWD/../Dfu/libDfu.a
}

include(../Flipartner_common.pri)

INCLUDEPATH += \
    $$PWD/../Dfu \
    $$PWD/../Backend

DEPENDPATH += \
    $$PWD/../Dfu \
    $$PWD/../Backend
