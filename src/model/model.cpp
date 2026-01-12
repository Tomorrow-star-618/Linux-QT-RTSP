#include "model.h"
#include "FFmpegDecoder.h"
#include <QDateTime>

#ifdef PLATFORM_RK3576
#include "MppDecoder.h"
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

Model::Model(QObject* parent)
    : QThread(parent), m_stop(false)
{
}

Model::~Model()
{
    stopStream();
    wait();
}

// 启动视频流线程
void Model::startStream(const QString& url)
{
    QMutexLocker locker(&m_mutex); // 加锁，保证线程安全
    m_url = url;                   // 设置RTSP流地址
    m_stop = false;                // 标记为未停止
    if (!isRunning())              // 如果线程未运行，则启动线程
        start();
    else                           // 如果线程已在运行，则唤醒等待的线程
        m_wait.wakeOne();
}

// 停止视频流线程
void Model::stopStream()
{
    QMutexLocker locker(&m_mutex); // 加锁，保证线程安全
    m_stop = true;                 // 标记为停止
    m_wait.wakeOne();              // 唤醒线程以便及时退出
}

// 暂停视频流
void Model::pauseStream()
{
    QMutexLocker locker(&m_mutex); // 加锁，保证线程安全
    m_pause = true;                // 设置暂停标志
}

// 恢复视频流
void Model::resumeStream()
{
    QMutexLocker locker(&m_mutex); // 加锁，保证线程安全
    m_pause = false;               // 取消暂停标志
    m_wait.wakeOne();              // 唤醒线程继续处理
}

// 打开RTSP流，获取AVFormatContext
bool Model::openStream(const QString& url, AVFormatContext*& fmt_ctx) {
    // 尝试打开输入流
    if (avformat_open_input(&fmt_ctx, url.toStdString().c_str(), nullptr, nullptr) != 0) {
        emit frameReady(QImage()); // 打开失败，发送空帧
        QThread::msleep(1000);     // 等待1秒后重试
        return false;
    }
    // 查找流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        avformat_close_input(&fmt_ctx);
        emit frameReady(QImage());
        QThread::msleep(1000);
        return false;
    }
    return true;
}

// 查找视频流索引
int Model::findVideoStream(AVFormatContext* fmt_ctx) {
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            return i; // 找到视频流
        }
    }
    return -1; // 未找到视频流
}

// 创建解码器（自动选择硬解或软解）
IVideoDecoder* Model::createDecoder()
{
#ifdef PLATFORM_RK3576
    qDebug() << "Model: 使用 MPP 硬件解码器";
    return new MppDecoder();
#else
    qDebug() << "Model: 使用 FFmpeg 软件解码器";
    return new FFmpegDecoder();
#endif
}

// 打开解码器
bool Model::openDecoder(AVFormatContext* fmt_ctx, int videoStream, IVideoDecoder*& decoder) {
    AVCodecParameters* codecpar = fmt_ctx->streams[videoStream]->codecpar;
    
    decoder = createDecoder();
    if (!decoder) {
        qWarning() << "Model: 创建解码器失败";
        return false;
    }
    
    if (!decoder->init(codecpar)) {
        qWarning() << "Model: 初始化解码器失败";
        delete decoder;
        decoder = nullptr;
        return false;
    }
    
    return true;
}

// 读取并解码视频帧，转换为QImage并发送信号
void Model::readAndDecodeFrames(AVFormatContext* fmt_ctx, IVideoDecoder* decoder, int videoStream) {
    AVPacket pkt;
    int readResult = 0;
    int frameCount = 0;
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    
    // 读取视频帧主循环
    while (!m_stop) {
        readResult = av_read_frame(fmt_ctx, &pkt);
        
        // 如果读取失败（推流端断开或其他错误）
        if (readResult < 0) {
            // 检查是否是EOF或连接断开
            if (readResult == AVERROR_EOF || readResult == AVERROR(EIO) || 
                readResult == AVERROR(EPIPE) || readResult == AVERROR(ECONNRESET)) {
                // 流结束或连接断开，正常退出循环，准备重连
                break;
            }
            // 其他错误，稍作延迟后继续尝试
            QThread::msleep(100);
            continue;
        }
        
        m_mutex.lock();
        if (m_pause) {
            m_wait.wait(&m_mutex); // 如果暂停，等待唤醒
        }
        m_mutex.unlock();
        
        if (pkt.stream_index == videoStream) {
            // 发送包到解码器
            if (decoder->sendPacket(&pkt)) {
                // 接收解码帧
                QImage img;
                while (decoder->receiveFrame(img)) {
                    emit frameReady(img);
                    frameCount++;
                    
                    // 每100帧统计一次帧率
                    if (frameCount % 100 == 0) {
                        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
                        qint64 elapsed = currentTime - startTime;
                        double fps = (frameCount * 1000.0) / elapsed;
                        qDebug() << "解码帧率:" << QString::number(fps, 'f', 2) << "fps，已解码" << frameCount << "帧";
                    }
                }
            }
        }
        av_packet_unref(&pkt); // 释放包
        
        m_mutex.lock();
        if (m_stop) {
            m_mutex.unlock();
            break; // 如果需要停止，跳出循环
        }
        m_mutex.unlock();
    }
}

// 释放所有相关资源
void Model::cleanup(AVFormatContext* fmt_ctx, IVideoDecoder* decoder) {
    if (decoder) {
        decoder->cleanup();
        delete decoder;
    }
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
}

// 主线程函数，负责整体流程调度
void Model::run()
{
    avformat_network_init(); // 初始化网络模块
    while (true)
    {
        // 获取当前url和停止标志
        m_mutex.lock();
        QString url = m_url;
        bool stop = m_stop;
        m_mutex.unlock();
        if (stop)
            break; // 需要停止时退出主循环
        AVFormatContext* fmt_ctx = nullptr;
        // 打开流
        if (!openStream(url, fmt_ctx))
            continue;
        // 查找视频流索引
        int videoStream = findVideoStream(fmt_ctx);
        if (videoStream == -1) {
            cleanup(fmt_ctx, nullptr);
            emit frameReady(QImage());
            QThread::msleep(1000);
            continue;
        }
        // 打开解码器
        IVideoDecoder* decoder = nullptr;
        if (!openDecoder(fmt_ctx, videoStream, decoder)) {
            cleanup(fmt_ctx, decoder);
            emit frameReady(QImage());
            QThread::msleep(1000);
            continue;
        }
        // 读取并解码帧
        readAndDecodeFrames(fmt_ctx, decoder, videoStream);
        
        // 释放资源
        cleanup(fmt_ctx, decoder);
        
        // 检查是否需要停止
        m_mutex.lock();
        bool shouldStop = m_stop;
        QString currentUrl = m_url;
        m_mutex.unlock();
        
        if (shouldStop) {
            break; // 如果需要停止，退出主循环
        }
        
        // 如果不是主动停止，说明是连接断开
        emit streamDisconnected(currentUrl);
        
        // 等待一段时间后尝试重连
        QThread::msleep(2000); // 等待2秒后重连
        
        // 发出重连信号
        emit streamReconnecting(currentUrl);
    }
} 