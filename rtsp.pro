QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

# 解决FFmpeg与标准库冲突的关键宏定义
DEFINES += __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS

SOURCES += \
    Picture.cpp \
    Tcpserver.cpp \
    VideoLabel.cpp \
    detectlist.cpp \
    main.cpp \
    mainwindow.cpp \
    model.cpp \
    view.cpp \
    controller.cpp \
    plan.cpp

HEADERS += \
    Picture.h \
    Tcpserver.h \
    VideoLabel.h \
    common.h \
    detectlist.h \
    mainwindow.h \
    model.h \
    view.h \
    controller.h \
    plan.h

FORMS += \
    mainwindow.ui

# FFmpeg 头文件路径 - 使用更合适的上级目录
INCLUDEPATH += /usr/include/x86_64-linux-gnu

# FFmpeg 库文件路径
LIBS += -L/usr/lib/x86_64-linux-gnu

# 链接 FFmpeg 的库
LIBS += -lavcodec -lavformat -lavutil -lswscale

## OpenCV 头文件路径
#INCLUDEPATH += /usr/include/opencv4

## OpenCV 库文件路径
#LIBS += -L/usr/lib/x86_64-linux-gnu

## 链接 OpenCV 的常用库
#LIBS += -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_videoio


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icon.qrc
