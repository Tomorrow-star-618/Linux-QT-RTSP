#pragma once
#include "IVideoDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

/**
 * @brief FFmpeg 软件解码器实现
 * 适用于 Windows、Ubuntu 等平台
 */
class FFmpegDecoder : public IVideoDecoder {
public:
    FFmpegDecoder();
    ~FFmpegDecoder() override;
    
    bool init(AVCodecParameters* codecpar) override;
    bool sendPacket(AVPacket* packet) override;
    bool receiveFrame(QImage& image) override;
    int getWidth() const override { return m_width; }
    int getHeight() const override { return m_height; }
    void cleanup() override;
    
private:
    AVCodecContext* m_codecCtx;
    AVFrame* m_frame;
    AVFrame* m_rgbFrame;
    SwsContext* m_swsCtx;
    uint8_t* m_buffer;
    int m_width;
    int m_height;
};
