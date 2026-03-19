#pragma once

#ifdef PLATFORM_RK3576

#include "IVideoDecoder.h"
#include <QDebug>

// MPP 和 FFmpeg 头文件需要 extern "C"
extern "C" {
#include <libavcodec/avcodec.h>
#include <rockchip/rk_mpi.h>
#include <rockchip/rk_type.h>
#include <rockchip/mpp_err.h>
#include <rockchip/mpp_frame.h>
#include <rockchip/mpp_packet.h>
}

// RGA 头文件不能放在 extern "C" 块中,因为包含 C++ inline 函数
#include <rga/rga.h>
#include <rga/RgaApi.h>

/**
 * @brief MPP 硬件解码器实现
 * 专用于 Rockchip RK3576 平台
 */
class MppDecoder : public IVideoDecoder {
public:
    MppDecoder();
    ~MppDecoder() override;
    
    bool init(AVCodecParameters* codecpar) override;
    bool sendPacket(AVPacket* packet) override;
    bool receiveFrame(QImage& image) override;
    int getWidth() const override { return m_width; }
    int getHeight() const override { return m_height; }
    void cleanup() override;
    
private:
    // 转换 YUV (NV12) 到 RGB - 使用 RGA 硬件加速
    bool convertNV12ToRGBbyRGA(MppFrame frame, QImage& image);
    // CPU 软件转换（备用）
    bool convertNV12ToRGBbyCPU(MppFrame frame, QImage& image);
    
    MppCtx m_mppCtx;           // MPP 上下文
    MppApi* m_mppApi;          // MPP API 接口
    MppBufferGroup m_frmGrp;   // 帧缓冲组
    
    int m_width;
    int m_height;
    
    // 增加硬件缩放目标尺寸
    int m_outWidth;
    int m_outHeight;

    bool m_initialized;
    bool m_useRGA;
    
    // 引入对象池，防止在使用零拷贝 `QImage` 时产生内存读写撕裂现象
    static const int POOL_SIZE = 4;
    uint8_t* m_rgbBuffers[POOL_SIZE];
    MppBuffer m_rgbMppBuffers[POOL_SIZE];
    int m_poolIndex;
    int m_rgbBufferSize;
};

#endif // PLATFORM_RK3576
