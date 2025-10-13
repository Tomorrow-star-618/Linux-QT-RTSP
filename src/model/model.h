#pragma once
#include <QObject>
#include <QImage>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class Model : public QThread {
    Q_OBJECT

public:
    explicit Model(QObject* parent = nullptr); // 构造函数，初始化Model对象
    ~Model();                                  // 析构函数，释放资源
    void startStream(const QString& url);      // 启动视频流线程，传入RTSP地址
    void stopStream();                         // 停止视频流线程
    void pauseStream();                        // 暂停视频流
    void resumeStream();                       // 恢复视频流
    bool isPaused() const { return m_pause; }  // 获取暂停状态

signals:
    void frameReady(const QImage& img);        // 视频帧准备好时发出信号，传递QImage

protected:
    void run() override;                       // 线程主函数，处理视频流解码

private:
    // 打开RTSP流，获取AVFormatContext
    bool openStream(const QString& url, AVFormatContext*& fmt_ctx);
    // 查找视频流索引
    int findVideoStream(AVFormatContext* fmt_ctx);
    // 打开解码器，获取AVCodecContext
    bool openDecoder(AVFormatContext* fmt_ctx, int videoStream, AVCodecContext*& codec_ctx);
    // 读取并解码视频帧，转换为QImage并发送信号
    void readAndDecodeFrames(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, int videoStream);
    // 释放所有相关资源
    void cleanup(AVFormatContext* fmt_ctx, AVCodecContext* codec_ctx, AVFrame* frame, AVFrame* rgbFrame, uint8_t* buffer, SwsContext* sws_ctx);
    QString m_url;             // RTSP流地址
    bool m_stop;               // 停止标志
    bool m_pause = false;      // 暂停标志
    QMutex m_mutex;            // 互斥锁，保证多线程安全
    QWaitCondition m_wait;     // 条件变量，用于线程等待和唤醒
}; 
