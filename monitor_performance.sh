#!/bin/bash

# ============================================
# RK3576 RTSP 程序性能监控脚本
# 监控: CPU / GPU / NPU / 内存
# ============================================

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# 进程名
PROCESS_NAME="rtsp"

# 清屏函数
clear_screen() {
    printf "\033[H\033[J"
}

# 获取进程 PID
get_pid() {
    pgrep -x "$PROCESS_NAME" 2>/dev/null | head -1
}

# 获取进程 CPU 使用率
get_process_cpu() {
    local pid=$1
    if [ -n "$pid" ]; then
        ps -p "$pid" -o %cpu --no-headers 2>/dev/null | tr -d ' '
    else
        echo "N/A"
    fi
}

# 获取进程内存使用 (MB)
get_process_mem() {
    local pid=$1
    if [ -n "$pid" ]; then
        local mem_kb=$(ps -p "$pid" -o rss --no-headers 2>/dev/null | tr -d ' ')
        if [ -n "$mem_kb" ]; then
            echo "scale=1; $mem_kb / 1024" | bc
        else
            echo "N/A"
        fi
    else
        echo "N/A"
    fi
}

# 获取系统总 CPU 使用率
get_system_cpu() {
    local cpu_line=$(head -1 /proc/stat)
    local cpu_values=($cpu_line)
    local user=${cpu_values[1]}
    local nice=${cpu_values[2]}
    local system=${cpu_values[3]}
    local idle=${cpu_values[4]}
    local iowait=${cpu_values[5]}
    local irq=${cpu_values[6]}
    local softirq=${cpu_values[7]}
    
    local total=$((user + nice + system + idle + iowait + irq + softirq))
    local used=$((user + nice + system + irq + softirq))
    
    if [ -f /tmp/cpu_prev_total ] && [ -f /tmp/cpu_prev_used ]; then
        local prev_total=$(cat /tmp/cpu_prev_total)
        local prev_used=$(cat /tmp/cpu_prev_used)
        local diff_total=$((total - prev_total))
        local diff_used=$((used - prev_used))
        if [ $diff_total -gt 0 ]; then
            echo "scale=1; $diff_used * 100 / $diff_total" | bc
        else
            echo "0"
        fi
    else
        echo "0"
    fi
    
    echo $total > /tmp/cpu_prev_total
    echo $used > /tmp/cpu_prev_used
}

# 获取 GPU 使用率
get_gpu_usage() {
    local gpu_load=""
    for path in /sys/class/devfreq/*gpu*/load /sys/class/devfreq/fb000000.gpu/load; do
        if [ -f "$path" ]; then
            gpu_load=$(cat "$path" 2>/dev/null | cut -d'@' -f1)
            break
        fi
    done
    if [ -n "$gpu_load" ]; then echo "$gpu_load"; else echo "N/A"; fi
}

# 获取 GPU 频率
get_gpu_freq() {
    local freq=""
    for path in /sys/class/devfreq/*gpu*/cur_freq /sys/class/devfreq/fb000000.gpu/cur_freq; do
        if [ -f "$path" ]; then
            freq=$(cat "$path" 2>/dev/null)
            break
        fi
    done
    if [ -n "$freq" ] && [ "$freq" -gt 0 ] 2>/dev/null; then
        echo "scale=0; $freq / 1000000" | bc
    else
        echo "N/A"
    fi
}

# 获取 NPU 使用率
get_npu_usage() {
    local npu_load=""
    for path in /sys/class/devfreq/*npu*/load /sys/class/devfreq/fdab0000.npu/load /proc/rknpu/load; do
        if [ -f "$path" ]; then
            npu_load=$(cat "$path" 2>/dev/null | cut -d'@' -f1)
            break
        fi
    done
    if [ -n "$npu_load" ]; then echo "$npu_load"; else echo "N/A"; fi
}

# 获取 NPU 频率
get_npu_freq() {
    local freq=""
    for path in /sys/class/devfreq/*npu*/cur_freq /sys/class/devfreq/fdab0000.npu/cur_freq; do
        if [ -f "$path" ]; then
            freq=$(cat "$path" 2>/dev/null)
            break
        fi
    done
    if [ -n "$freq" ] && [ "$freq" -gt 0 ] 2>/dev/null; then
        echo "scale=0; $freq / 1000000" | bc
    else
        echo "N/A"
    fi
}

# 获取系统内存信息
get_memory_info() {
    local mem_total=$(grep MemTotal /proc/meminfo | awk '{print $2}')
    local mem_available=$(grep MemAvailable /proc/meminfo | awk '{print $2}')
    local mem_used=$((mem_total - mem_available))
    local total_mb=$(echo "scale=0; $mem_total / 1024" | bc)
    local used_mb=$(echo "scale=0; $mem_used / 1024" | bc)
    local percent=$(echo "scale=1; $mem_used * 100 / $mem_total" | bc)
    echo "$used_mb $total_mb $percent"
}

# 获取硬件状态
get_mpp_status() {
    if [ -c /dev/mpp_service ]; then echo -e "${GREEN}可用${NC}"; else echo -e "${RED}不可用${NC}"; fi
}

get_rga_status() {
    if [ -c /dev/rga ]; then echo -e "${GREEN}可用${NC}"; else echo -e "${RED}不可用${NC}"; fi
}

# 进度条
progress_bar() {
    local value=$1
    local max=$2
    local width=25
    
    if [ "$value" = "N/A" ]; then
        printf "[%-${width}s]" "N/A"
        return
    fi
    
    local filled=$(echo "scale=0; $value * $width / $max" | bc 2>/dev/null)
    [ -z "$filled" ] && filled=0
    [ $filled -lt 0 ] && filled=0
    [ $filled -gt $width ] && filled=$width
    local empty=$((width - filled))
    
    local color=$GREEN
    [ $(echo "$value > 70" | bc 2>/dev/null) -eq 1 ] && color=$RED
    [ $(echo "$value > 40" | bc 2>/dev/null) -eq 1 ] && [ $(echo "$value <= 70" | bc 2>/dev/null) -eq 1 ] && color=$YELLOW
    
    printf "${color}["
    for ((i=0; i<filled; i++)); do printf "█"; done
    for ((i=0; i<empty; i++)); do printf "░"; done
    printf "]${NC}"
}

# 主循环
main() {
    rm -f /tmp/cpu_prev_total /tmp/cpu_prev_used
    
    echo -e "${GREEN}╔════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║      RK3576 RTSP 性能监控工具              ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "硬件加速状态:"
    echo -e "  MPP (视频硬解): $(get_mpp_status)"
    echo -e "  RGA (图形加速): $(get_rga_status)"
    echo ""
    echo -e "${YELLOW}按 Ctrl+C 退出${NC}"
    sleep 2
    
    while true; do
        clear_screen
        
        local pid=$(get_pid)
        local timestamp=$(date '+%H:%M:%S')
        
        echo -e "${CYAN}╔═══════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${CYAN}║${NC}       ${YELLOW}RK3576 RTSP 性能监控${NC}              [$timestamp]       ${CYAN}║${NC}"
        echo -e "${CYAN}╠═══════════════════════════════════════════════════════════════╣${NC}"
        
        if [ -n "$pid" ]; then
            echo -e "${CYAN}║${NC}  ${GREEN}● 进程: 运行中${NC} (PID: $pid)                                    ${CYAN}║${NC}"
        else
            echo -e "${CYAN}║${NC}  ${RED}○ 进程: 未运行${NC} - 请先启动 ./build/rtsp                      ${CYAN}║${NC}"
        fi
        
        echo -e "${CYAN}╠═══════════════════════════════════════════════════════════════╣${NC}"
        
        # CPU
        local sys_cpu=$(get_system_cpu)
        local proc_cpu=$(get_process_cpu "$pid")
        echo -e "${CYAN}║${NC}  ${BLUE}【CPU】${NC}                                                        ${CYAN}║${NC}"
        printf "${CYAN}║${NC}    系统总计: %5.1f%%  " "$sys_cpu"
        progress_bar "$sys_cpu" 100
        printf "              ${CYAN}║${NC}\n"
        
        if [ "$proc_cpu" != "N/A" ]; then
            printf "${CYAN}║${NC}    RTSP进程: %5s%%  " "$proc_cpu"
            progress_bar "$proc_cpu" 100
            printf "              ${CYAN}║${NC}\n"
        fi
        
        echo -e "${CYAN}╠═══════════════════════════════════════════════════════════════╣${NC}"
        
        # GPU
        local gpu_usage=$(get_gpu_usage)
        local gpu_freq=$(get_gpu_freq)
        echo -e "${CYAN}║${NC}  ${BLUE}【GPU - Mali】${NC}                                                 ${CYAN}║${NC}"
        printf "${CYAN}║${NC}    使用率: %5s%%    " "$gpu_usage"
        progress_bar "$gpu_usage" 100
        printf "              ${CYAN}║${NC}\n"
        printf "${CYAN}║${NC}    当前频率: %-6s MHz                                         ${CYAN}║${NC}\n" "$gpu_freq"
        
        echo -e "${CYAN}╠═══════════════════════════════════════════════════════════════╣${NC}"
        
        # NPU
        local npu_usage=$(get_npu_usage)
        local npu_freq=$(get_npu_freq)
        echo -e "${CYAN}║${NC}  ${BLUE}【NPU - RKNPU】${NC}                                                ${CYAN}║${NC}"
        printf "${CYAN}║${NC}    使用率: %5s%%    " "$npu_usage"
        progress_bar "$npu_usage" 100
        printf "              ${CYAN}║${NC}\n"
        printf "${CYAN}║${NC}    当前频率: %-6s MHz                                         ${CYAN}║${NC}\n" "$npu_freq"
        
        echo -e "${CYAN}╠═══════════════════════════════════════════════════════════════╣${NC}"
        
        # 内存
        local mem_info=$(get_memory_info)
        local mem_used=$(echo $mem_info | awk '{print $1}')
        local mem_total=$(echo $mem_info | awk '{print $2}')
        local mem_percent=$(echo $mem_info | awk '{print $3}')
        local proc_mem=$(get_process_mem "$pid")
        
        echo -e "${CYAN}║${NC}  ${BLUE}【内存】${NC}                                                       ${CYAN}║${NC}"
        printf "${CYAN}║${NC}    系统: %5s / %s MB (%4.1f%%) " "$mem_used" "$mem_total" "$mem_percent"
        progress_bar "$mem_percent" 100
        printf "      ${CYAN}║${NC}\n"
        
        if [ "$proc_mem" != "N/A" ]; then
            printf "${CYAN}║${NC}    RTSP进程: %6s MB                                          ${CYAN}║${NC}\n" "$proc_mem"
        fi
        
        echo -e "${CYAN}╠═══════════════════════════════════════════════════════════════╣${NC}"
        echo -e "${CYAN}║${NC}  ${BLUE}【硬件加速】${NC}  MPP: $(get_mpp_status)   RGA: $(get_rga_status)                      ${CYAN}║${NC}"
        echo -e "${CYAN}╚═══════════════════════════════════════════════════════════════╝${NC}"
        
        echo ""
        echo -e "  ${YELLOW}Ctrl+C 退出${NC}  |  刷新间隔: 1秒"
        
        sleep 1
    done
}

# 清理
cleanup() {
    rm -f /tmp/cpu_prev_total /tmp/cpu_prev_used
    echo ""
    echo -e "${GREEN}监控已停止${NC}"
    exit 0
}

trap cleanup INT
main
