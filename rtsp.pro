QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

# 解决FFmpeg与标准库冲突的关键宏定义
DEFINES += __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS

# ============================================
# 平台特定配置 - 自动检测
# ============================================
linux {
    # 方法1: 检测 Rockchip 设备树（最准确）
    DEVICE_TREE = $$system(grep -i rockchip /proc/device-tree/compatible 2>/dev/null)
    !isEmpty(DEVICE_TREE) {
        message("✓ 检测到 Rockchip 平台 ($$DEVICE_TREE)，启用 PLATFORM_RK3576")
        DEFINES += PLATFORM_RK3576
    }
    # 方法2: 检测 ARM64 架构作为备选
    else:equals(QT_ARCH, arm64)|equals(QT_ARCH, arm) {
        message("⚠ 检测到 ARM 架构，假定为嵌入式 Linux 平台，启用 PLATFORM_RK3576")
        DEFINES += PLATFORM_RK3576
    }
    else {
        message("✓ 检测到标准 Linux 平台 (x86/x64)，不启用平台特定配置")
    }
}

win32 {
    message("✓ 检测到 Windows 平台，不启用平台特定配置")
}

# 手动强制控制（最高优先级，会覆盖自动检测）
# 强制启用: DEFINES += PLATFORM_RK3576
# 强制禁用: DEFINES -= PLATFORM_RK3576

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
    $$MODEL_DIR/FFmpegDecoder.cpp \
    $$VIEW_DIR/mainwindow.cpp \
    $$VIEW_DIR/Picture.cpp \
    $$VIEW_DIR/VideoLabel.cpp \
    $$VIEW_DIR/detectlist.cpp \
    $$VIEW_DIR/plan.cpp \
    $$VIEW_DIR/view.cpp \
    $$VIEW_DIR/AddCameraDialog.cpp \
    $$CONTROLLER_DIR/controller.cpp \
    $$CONTROLLER_DIR/Tcpserver.cpp

# RK3576 平台特定源文件
contains(DEFINES, PLATFORM_RK3576) {
    SOURCES += $$MODEL_DIR/MppDecoder.cpp
}

# ============================================
# 头文件配置
# ============================================
HEADERS += \
    $$MODEL_DIR/model.h \
    $$MODEL_DIR/common.h \
    $$MODEL_DIR/IVideoDecoder.h \
    $$MODEL_DIR/FFmpegDecoder.h \
    $$VIEW_DIR/mainwindow.h \
    $$VIEW_DIR/Picture.h \
    $$VIEW_DIR/VideoLabel.h \
    $$VIEW_DIR/detectlist.h \
    $$VIEW_DIR/plan.h \
    $$VIEW_DIR/view.h \
    $$VIEW_DIR/AddCameraDialog.h \
    $$CONTROLLER_DIR/controller.h \
    $$CONTROLLER_DIR/Tcpserver.h

# RK3576 平台特定头文件
contains(DEFINES, PLATFORM_RK3576) {
    HEADERS += $$MODEL_DIR/MppDecoder.h
}

# ============================================
# UI文件配置
# ============================================
FORMS += \
    $$VIEW_DIR/mainwindow.ui

# ============================================
# 平台相关配置
# ============================================

# Windows 平台配置
win32 {
    # FFmpeg 路径配置 - 使用系统安装的完整版 FFmpeg
    FFMPEG_PATH = D:/Qt/ffmpeg-7.1.1-full_build-shared
    
    # FFmpeg 头文件路径
    INCLUDEPATH += $$FFMPEG_PATH/include
    
    # FFmpeg 库文件路径
    LIBS += -L$$FFMPEG_PATH/lib
    
    # 链接 FFmpeg 的库 (Windows 使用 .lib 或直接链接 .dll)
    LIBS += -lavcodec \
            -lavformat \
            -lavutil \
            -lswscale \
            -lavdevice \
            -lswresample \
            -lavfilter \
            -lpostproc
    
    # 自动复制 FFmpeg DLL 到编译输出目录
    FFMPEG_DLLS = $$FFMPEG_PATH/bin/*.dll
    FFMPEG_DLLS ~= s,/,\\,g  # 将路径分隔符转换为 Windows 格式
    DESTDIR_WIN = $$OUT_PWD
    DESTDIR_WIN ~= s,/,\\,g
    
    # 使用 QMAKE_POST_LINK 在编译后自动复制 DLL
    QMAKE_POST_LINK += xcopy /Y /D \"$$FFMPEG_DLLS\" \"$$DESTDIR_WIN\" $$escape_expand(\\n\\t)
}

# Linux 平台配置
unix:!macx {
    # FFmpeg 头文件路径 - 使用更合适的上级目录
    INCLUDEPATH += /usr/include/x86_64-linux-gnu
    
    # FFmpeg 库文件路径
    LIBS += -L/usr/lib/x86_64-linux-gnu
    
    # 链接 FFmpeg 的库
    LIBS += -lavcodec -lavformat -lavutil -lswscale
    
    # RK3576 平台特定配置
    contains(DEFINES, PLATFORM_RK3576) {
        message("✓ 配置 MPP 硬件解码库")
        
        # MPP 和 RGA 头文件路径
        INCLUDEPATH += /usr/include/rockchip
        
        # 链接 MPP 和 RGA 库
        LIBS += -lrockchip_mpp -lrga
    }
}

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
