#include "mainwindow.h"
#include <QApplication>
#include <iostream>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    // 打印 FFmpeg 的版本信息
    std::cout << "FFmpeg version: " << av_version_info() << std::endl;
    // 初始化 FFmpeg 库
    av_register_all();
    // 检查 FFmpeg 是否正确初始化
    if (avformat_network_init() != 0) {
        std::cerr << "FFmpeg network initialization failed!" << std::endl;
        return -1;
    }
    std::cout << "FFmpeg libraries loaded successfully!" << std::endl;
    // 清理资源
    avformat_network_deinit();

    w.show();
    return a.exec();
}
