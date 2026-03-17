#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
#ifdef PLATFORM_RK3576
    // 解决 Rockchip RK3576 平台 libGL 驱动问题
    // 强制使用软件渲染,避免硬件加速导致的 GLX 错误
    qputenv("QT_XCB_GL_INTEGRATION", "none");
#endif
    
    QApplication a(argc, argv);
    MainWindow w;
    
#ifdef PLATFORM_RK3576
    // 在 RK3576 嵌入式平台上全屏显示
    w.showFullScreen();
#else
    w.show();
#endif

    return a.exec();
}
