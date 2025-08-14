# 🚀 瑞芯微RV1106智能识别终端

<div align="center">

[![Platform](https://img.shields.io/badge/platform-Linux-blue.svg)](https://www.linux.org/)
[![Qt](https://img.shields.io/badge/Qt-5.12.9-green.svg)](https://www.qt.io/)
[![FFmpeg](https://img.shields.io/badge/FFmpeg-Latest-red.svg)](https://ffmpeg.org/)
[![License](https://img.shields.io/badge/license-MIT-yellow.svg)](LICENSE)

</div>

---

## 📋 项目概述

项目由PC端Qt服务器与本地RV1106客户端组成，采用分布式架构设计。本代码是PC端Qt服务器实现，基于经典的MVC架构模式，提供完整的智能视觉监控解决方案。

---

## 🖥️ 主界面展示

<div align="center">
<img width="1300" height="679" alt="主界面展示" src="https://github.com/user-attachments/assets/fe58d10c-e5d2-42e9-bdc7-0fae05eb1a49" />
</div>

---

## ✨ 核心功能
### 🌐 TCP服务端
- **多客户端支持**: 可同时处理多个客户端连接
- **收发数据**: 可以指定发送任意数据到当前连接上的任意客户端上
- **IP地址管理**: 自动获取本地所有IPv4地址
- **连接状态监控**: 实时监控socket连接状态变化
- **AI数据解析**: 自动解析DETECTIONS格式的检测结果

### 📺 视频流
- **RTSP流解码**: 利用FFmpeg工具支持RTSP视频流的实时解码和播放
- **多线程处理**: 使用独立线程处理视频流，避免阻塞UI界面
- **实时帧输出**: 将解码后的视频帧转换为QImage格式供UI显示
- **视频流控制**: 支持启动/停止/暂停/恢复操作

### 📸 相册
- **双相册系统**: 截图相册 + 报警相册独立管理
- **图片缩放**: 支持用户鼠标滚轮缩放（0.2x-3.0x倍数）
- **精准定位**: 支持水平滑动条快速定位与数字跳转

### 🎮 舵机云台
- **四向控制**: 上下左右自由转动 + 复位功能
- **步进设置**: 滑动条与下拉框设置舵机步进值
- **TCP指令**: 通过网络发送舵机控制指令

### 🤖 AI识别
- **YOLOv5支持**: 标准COCO数据集80类对象识别
- **实时检测**: 视频流中的目标检测
- **结果显示**: 检测框和置信度显示

### 🎯 区域识别
- **区域识别**: 支持指定方框内的区域识别，过滤方框外的信息
- **鼠标绘制**: 支持通过鼠标拖拽绘制矩形框
- **实时预览**: 绘制过程中实时显示矩形框轮廓和尺寸
- **尺寸显示**: 显示矩形框的宽度和高度信息

### 🔍 物体识别
- **物体识别**: 支持识别指定coco数据集内的类别，可单选或者多选
- **智能搜索**: 实时搜索高亮匹配对象
- **批量操作**: 全选/清空/搜索功能
- **选择统计**: 实时显示已选择数量

### 📋 方案预选
- **SQLite数据库**: 本地化存储方案配置
- **一键配置**: 预设完整的监控方案，一键应用所有配置
- **方案管理**: 可创建、保存、编辑、删除多个监控方案

---

## 🛠️ 开发环境
| 组件 | 版本 | 说明 |
|------|------|------|
| **运行平台** | Qt Creator 4.12.2 | 集成开发环境 |
| **构建套件** | Qt 5.12.9 (MinGW 64bit) | UI框架与编译工具链 |
| **音视频库** | FFmpeg Latest | 音视频处理库文件 |
| **操作系统** | Linux | 目标运行平台 |

---

## 📚 使用教程

### 🔧 环境配置

#### 1️⃣ 安装Qt 5.12.9

在Linux环境下安装Qt 5.12.9开发环境：

```bash
# 下载Qt 5.12.9安装包
wget https://download.qt.io/archive/qt/5.12/5.12.9/qt-opensource-linux-x64-5.12.9.run

# 给安装包执行权限
chmod +x qt-opensource-linux-x64-5.12.9.run

# 运行安装程序
./qt-opensource-linux-x64-5.12.9.run
```

> 💡 **提示**: 安装过程中请选择Qt Creator和MinGW编译器

#### 2️⃣ 安装FFmpeg库文件

更新系统包列表并安装FFmpeg：

```bash
# 更新包列表
sudo apt update

# 安装FFmpeg及其开发库
sudo apt install ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
```

#### 3️⃣ 查找库文件安装路径

使用以下命令查找FFmpeg库文件的安装路径：

```bash
# 查找avcodec库路径
/sbin/ldconfig -p | grep avcodec

# 查找avformat库路径
/sbin/ldconfig -p | grep avformat

# 查找avutil库路径
/sbin/ldconfig -p | grep avutil

# 查找swscale库路径
/sbin/ldconfig -p | grep swscale
```

**示例输出**:
```
libavcodec.so.58 (libc6,x86-64) => /usr/lib/x86_64-linux-gnu/libavcodec.so.58
libavformat.so.58 (libc6,x86-64) => /usr/lib/x86_64-linux-gnu/libavformat.so.58
libavutil.so.56 (libc6,x86-64) => /usr/lib/x86_64-linux-gnu/libavutil.so.56
libswscale.so.5 (libc6,x86-64) => /usr/lib/x86_64-linux-gnu/libswscale.so.5
```

#### 4️⃣ 配置Qt项目文件

打开`rtsp.pro`文件，找到`# FFmpeg 库文件路径`注释部分，替换为实际的库文件路径：

```pro
# FFmpeg 库文件路径 (根据步骤3查找到的路径进行配置)
LIBS += -L/usr/lib/x86_64-linux-gnu/ -lavcodec -lavformat -lavutil -lswscale

# FFmpeg 头文件路径
INCLUDEPATH += /usr/include/x86_64-linux-gnu
INCLUDEPATH += /usr/include
```

### 🌐 网络配置

#### 5️⃣ 查询虚拟机IP地址

使用`ifconfig`命令查询当前虚拟机的IP地址：

```bash
ifconfig
```

**示例输出**:
```
ens33: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.1.156  netmask 255.255.255.0  broadcast 192.168.1.255
        ...
```

> ⚠️ **注意**: 确保虚拟机IP与RV1106设备IP在同一局域网下，推荐将虚拟机配置为**桥接模式**。

#### 6️⃣ 修改TCP服务器监听地址

打开`Tcpserver.cpp`文件，找到IP地址初始化代码：

```cpp
// 修改前
Ip_lineEdit = new QLineEdit("192.168.1.156");

// 修改为步骤5查询到的虚拟机IP地址
Ip_lineEdit = new QLineEdit("你的虚拟机IP地址");
```

**例如**:
```cpp
Ip_lineEdit = new QLineEdit("192.168.1.100");  // 替换为实际IP
```

### 🚀 运行与使用

#### 7️⃣ 启动应用程序

1. **编译项目**: 在Qt Creator中打开项目并编译
2. **运行程序**: 点击运行按钮启动应用
3. **选择配置方式**:

##### 方式一：手动添加摄像头
1. 点击"添加视频"按钮
2. 输入RTSP地址: `rtsp://RV1106地址/live/0`
3. 点击确认开始视频流播放

##### 方式二：使用方案预设
1. 点击"方案预选"按钮
2. 选择预设的监控方案
3. 点击"应用方案"一键配置

> 🔗 **RTSP地址格式**: `rtsp://192.168.1.100/live/0`  
> 其中`192.168.1.100`为RV1106设备的实际IP地址

### ✅ 验证安装

#### 检查项目清单

- [ ] Qt 5.12.9 安装完成
- [ ] FFmpeg库文件安装成功
- [ ] 库文件路径配置正确
- [ ] 虚拟机与RV1106在同一网络
- [ ] TCP服务器IP地址配置正确
- [ ] 项目编译无错误
- [ ] 视频流连接成功

### 🔧 常见问题

#### 编译错误
- **问题**: 找不到FFmpeg头文件
- **解决**: 检查INCLUDEPATH配置是否正确

#### 网络连接失败
- **问题**: 无法连接到RV1106设备
- **解决**: 检查IP地址和网络连通性

#### 视频流无法播放
- **问题**: RTSP流连接失败
- **解决**: 确认RV1106设备RTSP服务正常运行

---
## 🏗️ 框架设计

### 📊 MVC架构模式
项目采用经典的MVC（Model-View-Controller）架构模式，实现了良好的代码分层和职责分离：

#### 🗃️ Model层 (数据模型层)
- **model.cpp/h**: 视频流数据处理模型
  - RTSP流解码和处理
  - FFmpeg集成和多线程管理
  - 视频帧数据转换和输出
  
- **plan.cpp/h**: 方案预选数据模型
  - SQLite数据库操作
  - 方案配置的CRUD操作
  - 数据持久化管理

#### 🖼️ View层 (视图展示层)
- **mainwindow.ui/cpp/h**: 主界面视图
  - 整体布局和界面组织
  - 各功能模块的视图集成
  
- **Picture.cpp/h**: 相册视图组件
  - 图片浏览和管理界面
  - 双相册系统展示
  
- **VideoLabel.cpp/h**: 自定义视频显示组件
  - 视频流显示控件
  - 交互式矩形框绘制界面
  
- **detectlist.cpp/h**: 对象选择视图
  - COCO数据集对象列表界面
  - 搜索和批量操作界面

#### 🎮 Controller层 (控制逻辑层)
- **controller.cpp/h**: 主控制器
  - 业务逻辑协调和控制
  - 各模块间的通信协调
  - 事件处理和响应管理
  
- **Tcpserver.cpp/h**: TCP通信控制器
  - 网络通信控制逻辑
  - 客户端连接管理
  - 数据协议解析和处理

### ⚙️ 核心技术架构

#### 🧵 多线程架构
```
主UI线程
├── 视频解码线程 (Model)
├── TCP服务器线程 (TcpServerThread)
└── 数据库操作线程 (Plan)
```

#### 📡 信号槽通信机制
- **Model → Controller**: 视频帧数据传递
- **TcpServer → Controller**: 网络数据和连接状态
- **Plan → Controller**: 方案配置数据
- **Controller → View**: 界面更新和状态同步

#### 🌊 数据流架构
```
RTSP视频流 → FFmpeg解码 → QImage转换 → 界面显示
TCP数据 → 协议解析 → 业务处理 → 设备控制
用户操作 → 控制器处理 → 数据模型 → 界面反馈
```

---

## 📁 项目结构
```
rtsp/
├── 📦 核心架构文件
│   ├── main.cpp                # 程序入口
│   ├── controller.cpp/h        # MVC主控制器
│   ├── model.cpp/h            # 视频流数据模型
│   ├── view.cpp/h             # 视图基类
│   └── common.h               # 公共头文件和宏定义
│
├── 🖼️ 界面组件
│   ├── mainwindow.cpp/h/ui    # 主窗口界面
│   ├── Picture.cpp/h          # 相册浏览组件
│   ├── VideoLabel.cpp/h       # 自定义视频显示控件
│   ├── detectlist.cpp/h       # 对象选择列表组件
│   └── plan.cpp/h             # 方案预选管理界面
│
├── 🌐 网络通信
│   └── Tcpserver.cpp/h        # TCP服务器实现
│
├── 🎨 资源文件
│   ├── icon.qrc              # 图标资源文件
│   ├── icon/                 # 图标文件目录
│   │   ├── addvideo.png      # 添加视频图标
│   │   ├── AI.png            # AI功能图标
│   │   ├── album.png         # 相册图标
│   │   ├── screenshot.png    # 截图图标
│   │   └── ...               # 其他界面图标
│   └── picture/              # 图片存储目录
│       ├── save-picture/     # 截图相册目录
│       └── alarm-picture/    # 报警相册目录
│
├── ⚙️ 配置文件
│   ├── rtsp.pro              # Qt项目配置文件
│   ├── rtsp.pro.user         # Qt Creator用户配置
│   └── .gitignore            # Git忽略配置
│
└── 📖 文档
    └── README.md             # 项目说明文档
```

### 🔗 依赖关系图
```
controller (核心控制器)
├── model (视频流模型)
├── Tcpserver (网络通信)
├── plan (方案管理)
├── mainwindow (主界面)
├── Picture (相册组件)
├── VideoLabel (视频控件)
└── detectlist (对象选择)
```

### 🎯 关键设计模式

#### 👀 观察者模式
- 使用Qt信号槽机制实现组件间解耦
- Model层数据变化自动通知View层更新

#### 🏷️ 单例模式
- Controller作为系统唯一控制中心
- 确保各模块协调统一

#### 🏭 工厂模式
- 统一的组件创建和初始化流程
- 便于扩展新的功能模块

#### 📋 策略模式
- 方案预选功能采用策略模式
- 不同监控场景对应不同配置策略

---

## 🚀 应用场景

- 🏭 **智能监控**: 工业现场实时监控和目标检测
- 🏠 **安防系统**: 家庭/办公室安防监控
- 🤖 **机器视觉**: 自动化生产线质量检测
- 📊 **数据采集**: 视觉数据收集和分析

---

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

---

<div align="center">
<b>⭐ 如果这个项目对你有帮助，请给它一个星标！</b>
</div>
