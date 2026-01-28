# UDP广播设备发现协议对接指南

## 概述

本协议用于实现摄像头设备的自动发现与零配置接入。上位机（PC客户端）通过UDP广播发送设备发现请求，摄像头设备收到请求后通过UDP广播响应自身信息，从而实现设备的自动发现。

## 网络配置

| 参数 | 值 |
|------|-----|
| **协议** | UDP |
| **端口** | 8888 (可配置) |
| **广播地址** | 255.255.255.255 或 子网广播地址 |
| **数据格式** | JSON (UTF-8编码) |

---

## 消息类型

### 1. 设备发现请求 (discovery_request)

**发送方**：上位机（PC客户端）  
**接收方**：所有摄像头设备  
**触发时机**：
- 用户点击"自动发现"按钮
- 定时自动发送（默认每10秒一次）

**数据格式**：
```json
{
    "type": "discovery_request",
    "version": "1.0",
    "timestamp": "2026-01-28T15:45:00+08:00"
}
```

**字段说明**：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| type | string | 是 | 消息类型，固定为 `discovery_request` |
| version | string | 是 | 协议版本号 |
| timestamp | string | 否 | ISO 8601格式时间戳 |

---

### 2. 设备发现响应 (discovery_response)

**发送方**：摄像头设备  
**接收方**：上位机（PC客户端）  
**触发时机**：收到 `discovery_request` 消息后

**数据格式**：
```json
{
    "type": "discovery_response",
    "device_id": "CAM_001_A1B2C3D4",
    "device_name": "前门摄像头",
    "rtsp_url": "rtsp://192.168.1.100/live/0",
    "rtsp_port": 554,
    "ip_address": "192.168.1.100",
    "manufacturer": "MyCamera",
    "model": "MC-200",
    "firmware_version": "1.2.3"
}
```

**字段说明**：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| type | string | ✅是 | 消息类型，固定为 `discovery_response` |
| device_id | string | ✅是 | 设备唯一标识符（建议使用MAC地址或UUID） |
| device_name | string | ✅是 | 设备显示名称 |
| rtsp_url | string | ✅是 | 完整的RTSP流地址 |
| rtsp_port | number | 否 | RTSP端口号，默认554 |
| ip_address | string | 否 | 设备IP地址（如不提供，上位机将使用UDP包来源IP） |
| manufacturer | string | 否 | 设备制造商 |
| model | string | 否 | 设备型号 |
| firmware_version | string | 否 | 固件版本号 |

---

### 3. 心跳消息 (heartbeat)

**发送方**：摄像头设备  
**接收方**：上位机（PC客户端）  
**触发时机**：设备定期发送（建议每10-15秒一次）

**数据格式**：
```json
{
    "type": "heartbeat",
    "device_id": "CAM_001_A1B2C3D4",
    "status": "online"
}
```

**字段说明**：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| type | string | ✅是 | 消息类型，固定为 `heartbeat` |
| device_id | string | ✅是 | 设备唯一标识符 |
| status | string | 否 | 设备状态：`online` / `busy` / `error` |

---

### 4. 连接请求 (connection_request) 🆕

**发送方**：上位机（PC客户端）  
**接收方**：目标摄像头设备（单播）  
**触发时机**：用户在设备列表中选择设备并点击"连接"按钮

**数据格式**：
```json
{
    "type": "connection_request",
    "version": "1.0",
    "host_ip": "192.168.1.50",
    "tcp_port": 8080,
    "timestamp": "2026-01-28T16:20:00+08:00"
}
```

**字段说明**：

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| type | string | ✅是 | 消息类型，固定为 `connection_request` |
| version | string | 是 | 协议版本号 |
| host_ip | string | ✅是 | **上位机的IP地址**，设备需要连接到此地址 |
| tcp_port | number | ✅是 | **上位机的TCP监听端口**（默认8080） |
| timestamp | string | 否 | ISO 8601格式时间戳 |

**⚠️ 设备端处理流程**：

1. 设备收到 `connection_request` 消息
2. 从消息中提取 `host_ip` 和 `tcp_port`
3. **主动建立TCP连接**到 `host_ip:tcp_port`
4. TCP连接建立后，开始正常的TCP通信

---

## 设备端实现示例

### C语言示例 (Linux/嵌入式)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DISCOVERY_PORT 8888
#define BUFFER_SIZE 1024

// 设备信息
const char* DEVICE_ID = "CAM_001_A1B2C3D4";
const char* DEVICE_NAME = "前门摄像头";
const char* DEVICE_IP = "192.168.1.100";
const char* RTSP_URL = "rtsp://192.168.1.100/live/0";

// 构建发现响应JSON
void build_discovery_response(char* buffer, size_t size) {
    snprintf(buffer, size,
        "{"
        "\"type\":\"discovery_response\","
        "\"device_id\":\"%s\","
        "\"device_name\":\"%s\","
        "\"rtsp_url\":\"%s\","
        "\"rtsp_port\":554,"
        "\"ip_address\":\"%s\","
        "\"manufacturer\":\"MyCamera\","
        "\"model\":\"MC-200\","
        "\"firmware_version\":\"1.0.0\""
        "}",
        DEVICE_ID, DEVICE_NAME, RTSP_URL, DEVICE_IP
    );
}

// 构建心跳消息JSON
void build_heartbeat(char* buffer, size_t size) {
    snprintf(buffer, size,
        "{"
        "\"type\":\"heartbeat\","
        "\"device_id\":\"%s\","
        "\"status\":\"online\""
        "}",
        DEVICE_ID
    );
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char recv_buffer[BUFFER_SIZE];
    char send_buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);
    
    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 允许广播
    int broadcast_enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    
    // 允许端口复用
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // 绑定端口
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DISCOVERY_PORT);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("设备发现服务已启动，监听端口 %d\n", DISCOVERY_PORT);
    
    // 广播地址配置
    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    broadcast_addr.sin_port = htons(DISCOVERY_PORT);
    
    while (1) {
        // 接收UDP数据
        int recv_len = recvfrom(sockfd, recv_buffer, BUFFER_SIZE - 1, 0,
                                (struct sockaddr*)&client_addr, &addr_len);
        
        if (recv_len > 0) {
            recv_buffer[recv_len] = '\0';
            printf("收到消息: %s\n", recv_buffer);
            
            // 检查是否为发现请求
            if (strstr(recv_buffer, "\"type\":\"discovery_request\"") != NULL) {
                printf("收到发现请求，发送响应...\n");
                
                // 构建并发送响应
                build_discovery_response(send_buffer, BUFFER_SIZE);
                sendto(sockfd, send_buffer, strlen(send_buffer), 0,
                       (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
                
                printf("已发送发现响应\n");
            }
        }
    }
    
    close(sockfd);
    return 0;
}
```

### Python示例

```python
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
摄像头设备端 - UDP广播发现服务
"""

import socket
import json
import threading
import time

DISCOVERY_PORT = 8888
BUFFER_SIZE = 1024

# 设备配置
DEVICE_CONFIG = {
    "device_id": "CAM_001_A1B2C3D4",
    "device_name": "前门摄像头",
    "rtsp_url": "rtsp://192.168.1.100/live/0",
    "rtsp_port": 554,
    "ip_address": "192.168.1.100",
    "manufacturer": "MyCamera",
    "model": "MC-200",
    "firmware_version": "1.0.0"
}


def build_discovery_response():
    """构建发现响应消息"""
    response = {
        "type": "discovery_response",
        **DEVICE_CONFIG
    }
    return json.dumps(response, ensure_ascii=False)


def build_heartbeat():
    """构建心跳消息"""
    return json.dumps({
        "type": "heartbeat",
        "device_id": DEVICE_CONFIG["device_id"],
        "status": "online"
    }, ensure_ascii=False)


def handle_discovery(sock):
    """处理发现请求"""
    print(f"设备发现服务已启动，监听端口 {DISCOVERY_PORT}")
    
    while True:
        try:
            data, addr = sock.recvfrom(BUFFER_SIZE)
            message = data.decode('utf-8')
            print(f"收到消息 [{addr[0]}:{addr[1]}]: {message}")
            
            # 解析JSON
            try:
                msg = json.loads(message)
                msg_type = msg.get("type", "")
                
                if msg_type == "discovery_request":
                    print("收到发现请求，发送响应...")
                    response = build_discovery_response()
                    sock.sendto(response.encode('utf-8'), 
                               ('<broadcast>', DISCOVERY_PORT))
                    print("已发送发现响应")
                    
            except json.JSONDecodeError:
                print(f"JSON解析失败: {message}")
                
        except Exception as e:
            print(f"接收错误: {e}")


def send_heartbeat(sock):
    """定期发送心跳"""
    while True:
        time.sleep(15)  # 每15秒发送一次心跳
        try:
            heartbeat = build_heartbeat()
            sock.sendto(heartbeat.encode('utf-8'), 
                       ('<broadcast>', DISCOVERY_PORT))
            print("已发送心跳")
        except Exception as e:
            print(f"心跳发送失败: {e}")


def main():
    # 创建UDP套接字
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    # 绑定端口
    sock.bind(('', DISCOVERY_PORT))
    
    # 启动心跳线程
    heartbeat_thread = threading.Thread(target=send_heartbeat, args=(sock,))
    heartbeat_thread.daemon = True
    heartbeat_thread.start()
    
    # 主线程处理发现请求
    handle_discovery(sock)


if __name__ == "__main__":
    main()
```

---

## 时序图

```
┌─────────┐                              ┌─────────────┐
│ 上位机  │                              │  摄像头设备  │
└────┬────┘                              └──────┬──────┘
     │                                          │
     │  1. discovery_request (广播)            │
     │────────────────────────────────────────>│
     │                                          │
     │  2. discovery_response (广播)           │
     │<────────────────────────────────────────│
     │                                          │
     │  [设备被发现并显示在列表中]              │
     │                                          │
     │                                          │
     │  3. heartbeat (广播, 定期)              │
     │<────────────────────────────────────────│
     │                                          │
     │  [更新设备在线状态]                      │
     │                                          │
```

---

## 注意事项

### 1. 网络配置
- 确保设备和上位机在**同一局域网**内
- 检查防火墙是否允许 **UDP 8888** 端口的入站/出站流量
- 路由器/交换机需要允许广播流量

### 2. device_id 规范
- 建议使用 **MAC地址** 或 **UUID** 作为唯一标识
- 格式建议：`CAM_<序号>_<MAC后6位>` 或纯UUID
- 每台设备的 device_id 必须**全局唯一**

### 3. RTSP URL 格式
```
rtsp://[用户名:密码@]<IP地址>[:<端口>]/<路径>
```
**示例**：
- `rtsp://192.168.1.100/live/0` (无认证)
- `rtsp://admin:123456@192.168.1.100:554/stream1` (带认证)

### 4. 超时与重试
- 上位机会每隔 **30秒** 检查设备心跳
- 超过30秒未收到心跳的设备会被标记为**离线**
- 建议设备端每 **10-15秒** 发送一次心跳

### 5. 性能优化
- JSON消息保持精简，避免过大的数据包
- 心跳消息仅包含必要字段
- 避免在响应中包含大量冗余信息

---

## 调试工具

### 使用 netcat 测试发送发现请求
```bash
# Linux/Mac
echo '{"type":"discovery_request","version":"1.0"}' | nc -u -b 255.255.255.255 8888

# Windows (PowerShell)
# 使用 nmap 的 ncat 或安装 netcat for windows
```

### 使用 Wireshark 抓包
过滤条件：`udp.port == 8888`

---

## 常见问题

### Q1: 上位机发送发现请求，但设备端收不到？
**A**: 检查以下几点：
1. 两端是否在同一子网
2. 防火墙是否阻止了UDP 8888端口
3. 设备端是否正确绑定到 `0.0.0.0:8888`

### Q2: 设备端发送响应，但上位机显示不到？
**A**: 检查以下几点：
1. 响应JSON格式是否正确（特别是 `type` 和 `device_id` 字段）
2. 设备端是否向广播地址发送响应
3. 上位机是否正确监听8888端口

### Q3: 设备频繁上线/离线？
**A**: 
1. 增加心跳发送频率（如每5秒一次）
2. 检查网络丢包率
3. 增加上位机的超时容忍时间

---

## 版本历史

| 版本 | 日期 | 修改内容 |
|------|------|----------|
| 1.0 | 2026-01-28 | 初始版本 |
