#include "mainwindow.h"
#include "model.h"
#include "view.h"
#include "controller.h"
#include "Tcpserver.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_tcpServer(nullptr)
{
    // 设置主窗口大小为800x480
    this->resize(800, 480);

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
