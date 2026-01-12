#pragma once
#include <QImage>

extern "C" {
#include <libavcodec/avcodec.h>
}

/**
 * @brief 视频解码器抽象接口
 * 用于统一软解码（FFmpeg）和硬解码（MPP）的接口
 */
class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;
    
    /**
     * @brief 初始化解码器
     * @param codecpar 编解码参数
     * @return 成功返回 true
     */
    virtual bool init(AVCodecParameters* codecpar) = 0;
    
    /**
     * @brief 发送数据包到解码器
     * @param packet AVPacket 数据包
     * @return 成功返回 true
     */
    virtual bool sendPacket(AVPacket* packet) = 0;
    
    /**
     * @brief 接收解码后的帧
     * @param image 输出 QImage
     * @return 成功返回 true，无帧可用返回 false
     */
    virtual bool receiveFrame(QImage& image) = 0;
    
    /**
     * @brief 获取视频宽度
     */
    virtual int getWidth() const = 0;
    
    /**
     * @brief 获取视频高度
     */
    virtual int getHeight() const = 0;
    
    /**
     * @brief 释放资源
     */
    virtual void cleanup() = 0;
};
