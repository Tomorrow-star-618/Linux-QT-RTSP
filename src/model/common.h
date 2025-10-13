#pragma once
#include <QRect>

// 通用数据结构定义
struct RectangleBox {
    int x;          // 左上角x坐标
    int y;          // 左上角y坐标
    int width;      // 矩形宽度
    int height;     // 矩形高度
    
    RectangleBox() : x(0), y(0), width(0), height(0) {}
    RectangleBox(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
    
    // 转换为QRect
    QRect toQRect() const {
        return QRect(x, y, width, height);
    }
    
    // 从QRect构造
    static RectangleBox fromQRect(const QRect& rect) {
        return RectangleBox(rect.x(), rect.y(), rect.width(), rect.height());
    }
}; 

// 归一化矩形框结构体
struct NormalizedRectangleBox {
    float x;      // 左上角x归一化坐标 (0~1)
    float y;      // 左上角y归一化坐标 (0~1)
    float width;  // 归一化宽度 (0~1)
    float height; // 归一化高度 (0~1)
    NormalizedRectangleBox() : x(0), y(0), width(0), height(0) {}
    NormalizedRectangleBox(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
}; 
