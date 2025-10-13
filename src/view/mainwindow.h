#pragma once
#include <QMainWindow>

class Model;
class View;
class Controller;
class Tcpserver;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Tcpserver* m_tcpServer; // TCP服务器指针
};