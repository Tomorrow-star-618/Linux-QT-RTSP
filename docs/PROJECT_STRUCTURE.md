# RTSP多路视频监控系统 - 项目文件结构说明

本文档描述项目的源代码文件结构，帮助开发者快速了解每个文件的功能职责。

---

## 📁 项目架构

本项目采用 **MVC (Model-View-Controller)** 架构设计：

```
src/
├── main.cpp              # 程序入口
├── model/                # 模型层 - 数据处理和视频解码
├── view/                 # 视图层 - 用户界面
└── controller/           # 控制器层 - 业务逻辑和通信
```

---

## 📄 入口文件

| 文件 | 功能说明 |
|------|----------|
| `main.cpp` | **程序入口点**，创建QApplication和主窗口实例 |

---

## 📂 Model层 - 模型层 (`src/model/`)

负责视频流的解码和数据结构定义。

| 文件 | 功能说明 |
|------|----------|
| `model.h / model.cpp` | **视频流解码模块**<br>• 使用FFmpeg解码RTSP视频流<br>• 继承自QThread，支持多线程解码<br>• 提供启动/停止/暂停/恢复视频流的接口<br>• 通过信号`frameReady()`输出解码后的QImage帧 |
| `common.h` | **通用数据结构定义**<br>• `RectangleBox` - 矩形框结构（整数坐标）<br>• `NormalizedRectangleBox` - 归一化矩形框结构（0~1浮点坐标）<br>• 用于绘框、区域识别等功能 |

---

## 📂 View层 - 视图层 (`src/view/`)

负责用户界面的展示和交互。

### 主界面

| 文件 | 功能说明 |
|------|----------|
| `mainwindow.h / mainwindow.cpp` | **主窗口容器**<br>• 程序主窗口，整合Model、View、Controller<br>• 管理TCP服务器实例 |
| `mainwindow.ui` | **主窗口UI定义**（Qt Designer文件） |
| `view.h / view.cpp` | **主视图界面**<br>• 整体界面布局（左侧按钮、中间视频区、右侧控制）<br>• 多路视频流管理（1/4/9/16宫格布局切换）<br>• 云台控制、功能按钮、事件消息显示<br>• 绘框功能支持 |

### 视频显示组件

| 文件 | 功能说明 |
|------|----------|
| `VideoLabel.h / VideoLabel.cpp` | **自定义视频标签控件**<br>• 继承自QLabel，支持视频帧显示<br>• 鼠标绘制矩形框功能<br>• 悬停控制条（添加/暂停/截图/关闭按钮）<br>• 双击选中视频流 |

### 对话框和弹窗

| 文件 | 功能说明 |
|------|----------|
| `AddCameraDialog.h / AddCameraDialog.cpp` | **添加摄像头对话框**<br>• 输入RTSP地址、摄像头名称<br>• 选择摄像头位置（1-16）<br>• 支持"自动发现"按钮，调用UDP设备发现 |
| `DeviceDiscoveryDialog.h / DeviceDiscoveryDialog.cpp` | **设备自动发现对话框** 🆕<br>• 显示UDP广播发现的设备列表<br>• 扫描动画、设备在线状态显示<br>• 双击或选择设备快速接入 |
| `Picture.h / Picture.cpp` | **相册浏览窗口**<br>• 浏览截图/报警图片<br>• 支持滑动条导航、缩放、删除<br>• 按时间排序、按摄像头筛选 |
| `detectlist.h / detectlist.cpp` | **对象检测列表窗口**<br>• 展示COCO数据集80类对象<br>• 多选复选框，支持搜索过滤<br>• 全选/清空/应用功能 |
| `plan.h / plan.cpp` | **方案预选窗口**<br>• 方案管理（新建/保存/删除/应用）<br>• 使用SQLite数据库持久化存储<br>• 配置AI功能、区域识别、对象列表 |

---

## 📂 Controller层 - 控制器层 (`src/controller/`)

负责业务逻辑处理和网络通信。

### 核心控制器

| 文件 | 功能说明 |
|------|----------|
| `controller.h / controller.cpp` | **主控制器**<br>• MVC架构的控制器，连接Model和View<br>• 处理按钮点击事件（添加摄像头/暂停/截图/绘框/TCP等）<br>• 云台控制逻辑<br>• 多路视频流管理<br>• 报警图片自动保存 |

### 网络通信

| 文件 | 功能说明 |
|------|----------|
| `Tcpserver.h / Tcpserver.cpp` | **TCP通信服务器**<br>• 监听客户端连接<br>• 发送控制指令（云台、AI功能、矩形框等）<br>• 接收检测数据<br>• IP与摄像头ID绑定管理<br>• 定义设备ID和操作ID枚举 |
| `DeviceDiscovery.h / DeviceDiscovery.cpp` | **UDP设备发现模块** 🆕<br>• 监听UDP 8888端口<br>• 发送设备发现广播请求<br>• 解析设备响应JSON，维护设备列表<br>• 心跳检测，设备离线状态管理 |

---

## 🆕 新增文件说明

以下文件为UDP广播设备自动发现功能新增：

### DeviceDiscovery（设备发现核心）

**功能**：
1. 向局域网广播设备发现请求
2. 监听摄像头设备的响应
3. 维护已发现设备列表
4. 定期检查设备心跳，标记离线设备

**协议**：JSON格式，端口8888

### DeviceDiscoveryDialog（设备发现UI）

**功能**：
1. 显示发现的设备表格（状态、名称、IP、RTSP地址等）
2. 扫描动画进度条
3. 刷新/连接/取消按钮
4. 双击设备快速选择

---

## 📊 文件依赖关系图

```
main.cpp
    └── mainwindow
            ├── Model (视频解码)
            ├── View (界面)
            │     ├── VideoLabel (视频标签)
            │     ├── AddCameraDialog (添加摄像头)
            │     │     └── DeviceDiscoveryDialog (设备发现)
            │     │           └── DeviceDiscovery (UDP广播)
            │     ├── Picture (相册)
            │     ├── DetectList (对象列表)
            │     └── Plan (方案管理)
            ├── Controller (控制器)
            └── Tcpserver (TCP通信)
```

---

## 📝 编码规范

- **命名**：使用驼峰命名法（类名首字母大写，成员变量`m_`前缀）
- **注释**：头文件使用Doxygen风格注释
- **Qt规范**：信号/槽使用`Q_OBJECT`宏，信号以动词开头，槽函数以`on`开头

---

## 🔧 构建信息

- **构建系统**：qmake
- **项目文件**：`rtsp.pro`
- **依赖库**：
  - Qt5 (Core, GUI, Widgets, Network, SQL)
  - FFmpeg (avcodec, avformat, avutil, swscale)

---

*文档更新日期：2026-01-28*
