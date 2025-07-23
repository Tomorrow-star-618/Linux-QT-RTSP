#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class RtspThread : public QThread {
    Q_OBJECT
public:
    RtspThread(QObject* parent = nullptr);
    ~RtspThread();
    void startStream(const QString& url);
    void stopStream();
signals:
    void frameReady(const QImage& img);
protected:
    void run() override;
private:
    QString m_url;
    bool m_stop;
    QMutex m_mutex;
    QWaitCondition m_wait;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
private slots:
    void onAddCameraClicked();
    void onFrameReady(const QImage& img);
private:
    QLabel* videoLabel;
    QPushButton* addCameraBtn;
    RtspThread* rtspThread;
};