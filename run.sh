#!/bin/bash

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}======================================${NC}"
echo -e "${GREEN}  RTSP 项目编译运行脚本${NC}"
echo -e "${GREEN}======================================${NC}"

# 1. 进入 build 目录
echo -e "${YELLOW}[1/4] 进入 build 目录...${NC}"
cd build || { echo -e "${RED}错误: build 目录不存在${NC}"; exit 1; }

# 2. 运行 qmake
echo -e "${YELLOW}[2/4] 运行 qmake...${NC}"
qmake ..
if [ $? -ne 0 ]; then
    echo -e "${RED}错误: qmake 执行失败${NC}"
    cd ..
    exit 1
fi

# 3. 编译项目
echo -e "${YELLOW}[3/4] 编译项目...${NC}"
make
if [ $? -ne 0 ]; then
    echo -e "${RED}错误: 编译失败，退出脚本${NC}"
    cd ..
    exit 1
fi

echo -e "${GREEN}✓ 编译成功！${NC}"

# 4. 运行程序
echo -e "${YELLOW}[4/4] 运行程序...${NC}"
echo -e "${GREEN}======================================${NC}"
./rtsp

# 5. 程序结束后返回上级目录
echo -e "${GREEN}======================================${NC}"
echo -e "${YELLOW}程序已退出，返回项目根目录${NC}"
cd ..
