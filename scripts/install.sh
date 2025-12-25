#!/bin/bash

# FlexTools 安装脚本

set -e

# 颜色定义
FLEX_RED='\033[0;31m'
FLEX_GREEN='\033[0;32m'
FLEX_YELLOW='\033[1;33m'
FLEX_BLUE='\033[0;34m'
FLEX_CYAN='\033[0;36m'
FLEX_NC='\033[0m' # No Color

# FlexTools 横幅
print_flex_banner() {
    echo -e "${FLEX_CYAN}"
    echo "███████╗██╗     ███████╗██╗  ██╗████████╗ ██████╗  ██████╗ ██╗     ███████╗"
    echo "██╔════╝██║     ██╔════╝╚██╗██╔╝╚══██╔══╝██╔═══██╗██╔═══██╗██║     ██╔════╝"
    echo "█████╗  ██║     █████╗   ╚███╔╝    ██║   ██║   ██║██║   ██║██║     ███████╗"
    echo "██╔══╝  ██║     ██╔══╝   ██╔██╗    ██║   ██║   ██║██║   ██║██║     ╚════██║"
    echo "██║     ███████╗███████╗██╔╝ ██╗   ██║   ╚██████╔╝╚██████╔╝███████╗███████║"
    echo "╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝   ╚═╝    ╚═════╝  ╚═════╝ ╚══════╝╚══════╝"
    echo -e "${FLEX_NC}"
}

echo -e "${FLEX_GREEN}FlexTools - Linux 系统信息与日志收集工具${FLEX_NC}"
echo "======================================================"

# 检查是否以root运行
if [[ $EUID -ne 0 ]]; then
   echo -e "${FLEX_YELLOW}提示: 使用sudo运行以获得完整功能${FLEX_NC}"
   FLEX_USER_INSTALL=true
else
   FLEX_USER_INSTALL=false
fi

# 检查依赖
echo -e "\n${FLEX_GREEN}[1/4] 检查依赖...${FLEX_NC}"

FLEX_DEPENDENCIES=("g++" "cmake" "make" "pkg-config")
FLEX_MISSING_DEPS=()

for dep in "${FLEX_DEPENDENCIES[@]}"; do
    if ! command -v $dep &> /dev/null; then
        FLEX_MISSING_DEPS+=($dep)
    fi
done

if [ ${#FLEX_MISSING_DEPS[@]} -ne 0 ]; then
    echo -e "${FLEX_YELLOW}缺少以下依赖: ${FLEX_MISSING_DEPS[*]}${FLEX_NC}"
    
    if [ "$FLEX_USER_INSTALL" = false ]; then
        # 根据发行版安装依赖
        if command -v apt &> /dev/null; then
            echo "使用APT安装依赖..."
            apt update
            apt install -y ${FLEX_MISSING_DEPS[@]}
        elif command -v yum &> /dev/null; then
            echo "使用YUM安装依赖..."
            yum install -y ${FLEX_MISSING_DEPS[@]}
        elif command -v dnf &> /dev/null; then
            echo "使用DNF安装依赖..."
            dnf install -y ${FLEX_MISSING_DEPS[@]}
        elif command -v pacman &> /dev/null; then
            echo "使用Pacman安装依赖..."
            pacman -Syu --noconfirm ${FLEX_MISSING_DEPS[@]}
        else
            echo -e "${FLEX_RED}无法自动安装依赖，请手动安装${FLEX_NC}"
            exit 1
        fi
    else
        echo -e "${FLEX_YELLOW}请手动安装依赖包${FLEX_NC}"
        echo "对于Debian/Ubuntu: sudo apt install ${FLEX_MISSING_DEPS[@]}"
        echo "对于CentOS/RHEL: sudo yum install ${FLEX_MISSING_DEPS[@]}"
        echo "对于Fedora: sudo dnf install ${FLEX_MISSING_DEPS[@]}"
        echo "对于Arch: sudo pacman -S ${FLEX_MISSING_DEPS[@]}"
    fi
else
    echo "所有依赖已满足"
fi

# 创建构建目录
echo -e "\n${FLEX_GREEN}[2/4] 构建FlexTools...${FLEX_NC}"
FLEX_BUILD_DIR="build"
if [ -d "$FLEX_BUILD_DIR" ]; then
    echo -e "${FLEX_YELLOW}清理现有构建目录...${FLEX_NC}"
    rm -rf "$FLEX_BUILD_DIR"
fi

mkdir -p "$FLEX_BUILD_DIR"
cd "$FLEX_BUILD_DIR"

# 运行CMake
echo -e "${FLEX_BLUE}配置项目...${FLEX_NC}"
if [ "$FLEX_USER_INSTALL" = true ]; then
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/.local
else
    cmake .. -DCMAKE_BUILD_TYPE=Release
fi

# 编译
echo -e "${FLEX_BLUE}编译FlexTools...${FLEX_NC}"
make -j$(nproc)

# 安装
echo -e "\n${FLEX_GREEN}[3/4] 安装FlexTools...${FLEX_NC}"
if [ "$FLEX_USER_INSTALL" = true ]; then
    make install
    FLEX_INSTALL_DIR="$HOME/.local"
else
    make install
    FLEX_INSTALL_DIR="/usr/local"
fi

# 创建系统链接
echo -e "\n${FLEX_GREEN}[4/4] 配置系统...${FLEX_NC}"
if [ "$FLEX_USER_INSTALL" = false ]; then
    # 系统安装
    ln -sf /usr/local/bin/flextools /usr/bin/flextools 2>/dev/null || true
    echo -e "${FLEX_GREEN}系统链接已创建${FLEX_NC}"
else
    # 用户安装
    if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
        echo -e "${FLEX_YELLOW}已将 ~/.local/bin 添加到 PATH${FLEX_NC}"
        echo -e "${FLEX_YELLOW}请运行: source ~/.bashrc 或重新登录${FLEX_NC}"
    fi
fi

# 创建配置文件目录
FLEX_CONFIG_DIR="$HOME/.config/flextools"
if [ ! -d "$FLEX_CONFIG_DIR" ]; then
    mkdir -p "$FLEX_CONFIG_DIR"
    echo -e "${FLEX_BLUE}创建配置文件目录: $FLEX_CONFIG_DIR${FLEX_NC}"
fi

# 生成默认配置文件
if [ ! -f "$FLEX_CONFIG_DIR/flextools.conf" ]; then
    cat > "$FLEX_CONFIG_DIR/flextools.conf" << EOF
# FlexTools 配置文件
# 版本: 1.0.0

[general]
# 默认输出格式: text, json, csv
default_format = text

# 默认信息级别: basic, detailed, all
default_level = basic

# 颜色输出: true, false
color_output = true

# 缓存系统信息: true, false
enable_cache = true

# 缓存过期时间 (秒)
cache_ttl = 300

[logging]
# 日志级别: debug, info, warning, error, critical
log_level = info

# 日志文件路径
log_file = $HOME/.cache/flextools/flextools.log

# 最大日志文件大小 (MB)
max_log_size = 10

# 保留的日志文件数量
max_log_files = 5

[modules]
# 启用模块
enable_system = true
enable_hardware = true
enable_packages = true
enable_logs = true

# 包管理器的最大显示数量
max_packages_display = 50

# 日志文件的最大行数
max_log_lines = 1000

[security]
# 敏感信息过滤
filter_sensitive = true

# 需要过滤的关键词 (逗号分隔)
sensitive_keywords = password,secret,key,token,auth
EOF
    echo -e "${FLEX_BLUE}创建默认配置文件: $FLEX_CONFIG_DIR/flextools.conf${FLEX_NC}"
fi

print_flex_banner
echo -e "${FLEX_GREEN}FlexTools 安装完成!${FLEX_NC}"
echo ""
echo -e "${FLEX_CYAN}使用示例:${FLEX_NC}"
echo "  flextools --help                  # 显示帮助"
echo "  flextools --system --hardware     # 显示系统和硬件信息"
echo "  flextools --all --format json     # 显示所有信息并输出为JSON"
echo "  flextools --packages --output packages.txt  # 导出包信息到文件"
echo ""
echo -e "${FLEX_CYAN}配置文件位置:${FLEX_NC}"
echo "  $FLEX_CONFIG_DIR/flextools.conf"
echo ""
echo -e "${FLEX_GREEN}开始使用 FlexTools 探索您的系统吧!${FLEX_NC}"
