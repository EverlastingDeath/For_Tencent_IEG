QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    qffmpeg.cpp \
    rtspthread.cpp

HEADERS += \
    mainwindow.h \
    qffmpeg.h \
    rtspthread.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += /usr/local/include

LIBS += /usr/local/lib/libavformat.a  \
        /usr/local/lib/libavdevice.a \
        /usr/local/lib/libavcodec.a \
        /usr/local/lib/libavutil.a   \
        /usr/local/lib/libswresample.a  \
        /usr/local/lib/libavfilter.a \
        /usr/local/lib/libswscale.a \
        /usr/local/lib/libx264.a

QMAKE_LFLAGS +=-no-pie
