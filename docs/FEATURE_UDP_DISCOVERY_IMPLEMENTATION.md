# UDP设备自动发现与零配置接入 - 技术实现详解

**文档版本**：1.1
**更新日期**：2026-01-28
**对应模块**：`DeviceDiscovery` / `DeviceDiscoveryDialog`

---

## 1. 功能概述

本功能旨在解决传统网络监控系统中"配置繁琐"的痛点。通过实现基于UDP广播的私有发现协议，系统能够自动扫描同一局域网内的摄像头设备，获取其RTSP流地址、IP、型号等信息。用户只需通过图形化界面点击"连接"，设备即可主动与上位机建立TCP控制通道，实现"即插即用"的零配置体验。

---

## 2. 系统架构设计

系统采用经典的 MVC (Model-View-Controller) 分层架构，确保逻辑与界面分离，便于维护和扩展。

### 2.1 模块依赖关系

```
[View层] DeviceDiscoveryDialog
       ↓ (调用)
[Controller层] DeviceDiscovery <---> [Network] UDP Socket (Port 8888)
       ↓ (信号)
[Model层] DiscoveredDevice (结构体数据)
```

### 2.2 核心类详解

#### 1. `DeviceDiscovery` (控制器核心)
- **文件位置**：`src/controller/DeviceDiscovery.h/cpp`
- **主要职责**：
  - **Socket管理**：维护一个 `QUdpSocket`，绑定到 `0.0.0.0:8888`。
  - **广播机制**：支持向 `255.255.255.255` 及所有网络接口的广播地址发送数据。
  - **协议解析**：通过 `QJsonDocument` 解析收到的 JSON 数据包，验证字段完整性。
  - **设备维护**：使用 `QMap<QString, DiscoveredDevice>` 维护设备列表，自带去重功能。
  - **心跳监测**：启动 `QTimer` (每5秒)，检查设备 `lastSeen` 时间，超过30秒未响应标记为离线。
  - **智能IP选择**：实现了核心算法，用于在多网卡环境下寻找与设备通信的最佳本机IP。

#### 2. `DeviceDiscoveryDialog` (交互界面)
- **文件位置**：`src/view/DeviceDiscoveryDialog.h/cpp`
- **主要职责**：
  - **列表展示**：使用 `QTableWidget` 展示设备信息，自定义行列样式。
  - **状态反馈**：通过红/绿点 (🔴/🟢) 实时反馈设备在线状态。
  - **动画效果**：扫描时显示进度条循环动画，提升用户体验。
  - **交互逻辑**：处理"刷新"、"连接"按钮事件，双击事件等。

---

## 3. 通信协议详解

所有通信数据均采用 **JSON 格式**，编码为 **UTF-8**。
- **传输层协议**：UDP
- **通信端口**：8888 (发现/心跳), 8890 (TCP控制连接)

### 3.1 协议消息类型汇总

| 消息类型 (`type`) | 方向 | 方式 | 作用 |
|-------------------|------|------|------|
| `discovery_request` | PC -> CAM | 广播 | 询问局域网内有哪些设备在线 |
| `discovery_response` | CAM -> PC | 广播 | 设备汇报自身详细信息 (IP, RTSP等) |
| `heartbeat` | CAM -> PC | 广播 | 设备周期性汇报状态，维持在线判定 |
| `connection_request` | PC -> CAM | **单播** | 通知设备主动反向连接上位机TCP服务 |

### 3.2 详细报文格式

#### (1) 设备发现请求
上位机发出，触发所有设备响应。
```json
{
    "type": "discovery_request",
    "version": "1.0",
    "timestamp": "2026-01-28T17:40:00+08:00"
}
```

#### (2) 设备发现响应 & 信息上报
设备收到请求后立即发送，或设备冷启动时主动发送。
```json
{
    "type": "discovery_response",
    "device_id": "CAM_001_A1B2C3",      // 唯一ID，推荐MAC地址
    "device_name": "前门高清监控",
    "rtsp_url": "rtsp://192.168.1.100/live/0", // 默认RTSP地址
    "rtsp_port": 554,
    "ip_address": "192.168.1.100",
    "manufacturer": "MySmartCam",
    "model": "Pro-2026",
    "firmware_version": "v1.2.0"
}
```

#### (3) 连接请求 (关键业务)
用户点击连接时发送，包含上位机的连接参数。
```json
{
    "type": "connection_request",
    "version": "1.0",
    "host_ip": "192.168.1.182",   // 智能计算得出的上位机实际局域网IP
    "tcp_port": 8890,             // 上位机监听的TCP端口（固定）
    "timestamp": "2026-01-28T17:42:00+08:00"
}
```
> **注意**：`host_ip` 的准确性至关重要，详见下文技术难点解析。

---

## 4. 关键技术难点与解决方案

在开发过程中解决的三个核心技术问题，是本功能稳定运行的基础。

### 4.1 多网卡环境下的智能IP选择算法

**背景**：
在安装了 VMware、VirtualBox、Docker 或启用了 Hyper-V 的开发机上，通常存在多个虚拟网卡。如果在发送 `connection_request` 时简单地取第一个非回环IP（例如 `198.18.0.1`），设备（IP为 `192.168.1.100`）将无法通过该虚拟IP反向连接到上位机。

**解决方案代码逻辑**：
```cpp
// 伪代码逻辑演示
QString selectBestLocalIp(QString deviceIp) {
    targetAddr = QHostAddress(deviceIp);
    
    foreach (interface in allInterfaces) {
        // 1. 过滤掉未启用、回环接口
        if (!Up || !Running || Loopback) continue;
        
        // 2. 黑名单过滤虚拟网卡名称
        name = interface.name().toLower();
        if (name.contains("vmware") || name.contains("docker") || 
            name.contains("virtualbox") || name.contains("vethernet")) {
            continue; 
        }

        foreach (entry in interface.addressEntries) {
            // 3. 核心：位运算匹配子网
            if ((entry.ip & entry.netmask) == (targetAddr & entry.netmask)) {
                return entry.ip; // 🎯 找到同一物理网段的IP，直接返回
            }
            // 4. 收集私有网段IP作为备选 (192.168.x.x, 10.x.x.x)
            if (isPrivateIp(entry.ip)) candidates.append(entry.ip);
        }
    }
    // 5. 兜底：如果没找到同网段，返回第一个候选的私有IP
    return candidates.first();
}
```

### 4.2 RTSP URL 的兼容性构造

**背景**：
部分流媒体播放库（如 FFmpeg 的某些构建版本）对 RTSP URL 格式非常敏感。如果 URL 为 `rtsp://192.168.1.100:554/live/0`，显式带上 554 端口可能导致连接超时或解析错误；而 UDP 广播包中的 `ip_address` 字段有时不准确。

**解决方案**：
1. **信任物理来源**：忽略 JSON 包里的 `ip_address`，直接使用 UDP 报文头的 `Sender IP`。
2. **清洗 IPv6 映射**：如果是 IPv4 映射地址（如 `::ffff:192.168.1.100`），强制截取后段转为纯 IPv4。
3. **智能端口处理**：
   - 端口 == 554：生成 `rtsp://192.168.1.100/live/0` (省略端口，最通用)
   - 端口 != 554：生成 `rtsp://192.168.1.100:8554/live/0` (必须带端口)

### 4.3 端口统一管理

为避免端口冲突和配置混乱，项目中明确规定了端口用途：

| 端口号 | 协议 | 用途 | 配置位置 |
|--------|------|------|----------|
| **8888** | UDP | 设备发现、心跳保活 | `DeviceDiscovery::DEFAULT_DISCOVERY_PORT` |
| **8890** | TCP | 设备控制、参数设置、反向连接 | `DeviceDiscoveryDialog` (常量 tcpPort) |
| **554** | TCP/UDP | RTSP 视频流传输 | 标准协议默认端口 |

---

## 5. 调试与排查指南

### 5.1 常用调试手段

1. **上位机日志**：查看 Qt Creator 的 Application Output 窗口。
   - 搜索关键字 `DeviceDiscovery:` 可查看所有发现层的日志。
   - 成功日志示例：`DeviceDiscovery: 已向 192.168.1.100 发送连接请求，上位机IP: 192.168.1.182`

2. **Wireshark 抓包**：
   - 过滤器：`udp.port == 8888`
   - 观察是否收到 `discovery_response` 包。
   - 观察点击连接后是否发出 `connection_request` 单播包。

### 5.2 常见问题速查

- **Q: 搜不到设备？**
  - A: 检查防火墙是否放行了 UDP 8888 端口；确认设备与电脑在同一物理局域网（非 NAT 隔离）。

- **Q: 搜到了但连接不上？**
  - A: 检查日志中发送的 `host_ip` 是否正确（是否误发了虚拟网卡IP）；检查设备端是否成功收到了 UDP 单播包。

- **Q: 连接上了但没视频？**
  - A: 检查生成的 RTSP URL 是否正确（是否多了端口号或 IP 格式不对）；使用 VLC 播放器手动测试该 RTSP 地址。

---
