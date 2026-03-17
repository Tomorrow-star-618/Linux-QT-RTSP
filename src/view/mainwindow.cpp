#include "mainwindow.h"
#include "model.h"
#include "view.h"
#include "controller.h"
#include "Tcpserver.h"
#include <QDebug>
#include <QApplication>
#include <QIcon>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_tcpServer(nullptr)
{
#ifdef PLATFORM_RK3576
    // 嵌入式平台：动态获取物理屏幕的真实分辨率并强制自适应铺满
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        this->setGeometry(screen->geometry());
    }
#else
    // PC开发环境：保持适合调试的窗口大小
    this->resize(900, 550);
#endif
    
    // 设置窗口图标
    this->setWindowIcon(QIcon(":icon/lmx.png"));
    
    // 设置应用程序图标（在任务栏等地方显示）
    QApplication::setWindowIcon(QIcon(":icon/lmx.png"));

    // 创建TCP服务器并自动启动监听
    m_tcpServer = new Tcpserver();
    m_tcpServer->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除
    // 隐藏TCP服务器窗口
    m_tcpServer->hide();
    // 自动启动TCP监听
    m_tcpServer->startListen();
    qDebug() << "TCP服务器已自动启动并开始监听";
    
    // 创建模型对象，父对象为MainWindow
    Model *model = new Model(this);
    // 创建视图对象，父对象为MainWindow
    View *view = new View(this);
    // 创建控制器对象，关联模型和视图，父对象为MainWindow
    Controller *controller = new Controller(model, view, this);

    // 将TCP服务器指针传递给Controller
    controller->setTcpServer(m_tcpServer);

    // 防止未使用变量警告
    Q_UNUSED(controller);
    // 设置视图为主窗口的中心部件
    this->setCentralWidget(view);
}

MainWindow::~MainWindow()
{
    // 清理TCP服务器
    if (m_tcpServer) {
        m_tcpServer->stopListen();
        delete m_tcpServer;
        m_tcpServer = nullptr;
    }
}
