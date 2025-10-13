QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

# 解决FFmpeg与标准库冲突的关键宏定义
DEFINES += __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS

# ============================================
# 源代码目录定义
# ============================================
SOURCES_DIR = $$PWD/src
MODEL_DIR = $$SOURCES_DIR/model
VIEW_DIR = $$SOURCES_DIR/view
CONTROLLER_DIR = $$SOURCES_DIR/controller

# 头文件搜索路径 - 添加各层目录到包含路径
INCLUDEPATH += $$MODEL_DIR \
               $$VIEW_DIR \
               $$CONTROLLER_DIR

# ============================================
# 源文件配置
# ============================================
SOURCES += \
    $$SOURCES_DIR/main.cpp \
    $$MODEL_DIR/model.cpp \
    $$VIEW_DIR/mainwindow.cpp \
    $$VIEW_DIR/Picture.cpp \
    $$VIEW_DIR/VideoLabel.cpp \
    $$VIEW_DIR/detectlist.cpp \
    $$VIEW_DIR/plan.cpp \
    $$VIEW_DIR/view.cpp \
    $$VIEW_DIR/AddCameraDialog.cpp \
    $$CONTROLLER_DIR/controller.cpp \
    $$CONTROLLER_DIR/Tcpserver.cpp

# ============================================
# 头文件配置
# ============================================
HEADERS += \
    $$MODEL_DIR/model.h \
    $$MODEL_DIR/common.h \
    $$VIEW_DIR/mainwindow.h \
    $$VIEW_DIR/Picture.h \
    $$VIEW_DIR/VideoLabel.h \
    $$VIEW_DIR/detectlist.h \
    $$VIEW_DIR/plan.h \
    $$VIEW_DIR/view.h \
    $$VIEW_DIR/AddCameraDialog.h \
    $$CONTROLLER_DIR/controller.h \
    $$CONTROLLER_DIR/Tcpserver.h

# ============================================
# UI文件配置
# ============================================
FORMS += \
    $$VIEW_DIR/mainwindow.ui

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
