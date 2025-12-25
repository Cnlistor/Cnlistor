#!/bin/bash

# FlexTools 卸载脚本

set -e

# 颜色定义
FLEX_RED='\033[0;31m'
FLEX_GREEN='\033[0;32m'
FLEX_YELLOW='\033[1;33m'
FLEX_NC='\033[0m' # No Color

echo -e "${FLEX_YELLOW}FlexTools 卸载脚本${FLEX_NC}"
echo "=============================="

# 检查是否以root运行
if [[ $EUID -eq 0 ]]; then
    FLEX_INSTALL_DIR="/usr/local"
    FLEX_BIN_LINK="/usr/bin/flextools"
else
    FLEX_INSTALL_DIR="$HOME/.local"
    FLEX_BIN_LINK=""
fi

# 确认卸载
read -p "确定要卸载 FlexTools? (y/n): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${FLEX_GREEN}取消卸载${FLEX_NC}"
    exit 0
fi

# 卸载步骤
echo -e "\n${FLEX_YELLOW}[1/4] 移除可执行文件...${FLEX_NC}"

# 移除可执行文件
if [ -f "$FLEX_INSTALL_DIR/bin/flextools" ]; then
    rm -f "$FLEX_INSTALL_DIR/bin/flextools"
    echo -e "${FLEX_GREEN}已移除: $FLEX_INSTALL_DIR/bin/flextools${FLEX_NC}"
fi

# 移除系统链接
if [ -n "$FLEX_BIN_LINK" ] && [ -L "$FLEX_BIN_LINK" ]; then
    rm -f "$FLEX_BIN_LINK"
    echo -e "${FLEX_GREEN}已移除系统链接: $FLEX_BIN_LINK${FLEX_NC}"
fi

echo -e "\n${FLEX_YELLOW}[2/4] 移除头文件...${FLEX_NC}"

# 移除头文件
FLEX_INCLUDE_DIR="$FLEX_INSTALL_DIR/include/flextools"
if [ -d "$FLEX_INCLUDE_DIR" ]; then
    rm -rf "$FLEX_INCLUDE_DIR"
    echo -e "${FLEX_GREEN}已移除头文件目录: $FLEX_INCLUDE_DIR${FLEX_NC}"
fi

echo -e "\n${FLEX_YELLOW}[3/4] 移除CMake配置文件...${FLEX_NC}"

# 移除CMake配置文件
FLEX_CMAKE_DIR="$FLEX_INSTALL_DIR/lib/cmake/FlexTools"
if [ -d "$FLEX_CMAKE_DIR" ]; then
    rm -rf "$FLEX_CMAKE_DIR"
    echo -e "${FLEX_GREEN}已移除CMake配置: $FLEX_CMAKE_DIR${FLEX_NC}"
fi

echo -e "\n${FLEX_YELLOW}[4/4] 清理用户配置和缓存...${FLEX_NC}"

# 清理用户配置 (可选)
read -p "是否删除用户配置和缓存文件? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    # 配置文件
    if [ -d "$HOME/.config/flextools" ]; then
        rm -rf "$HOME/.config/flextools"
        echo -e "${FLEX_GREEN}已移除配置文件: $HOME/.config/flextools${FLEX_NC}"
    fi
    
    # 缓存文件
    if [ -d "$HOME/.cache/flextools" ]; then
        rm -rf "$HOME/.cache/flextools"
        echo -e "${FLEX_GREEN}已移除缓存文件: $HOME/.cache/flextools${FLEX_NC}"
    fi
    
    # 从PATH中移除 (用户安装时)
    if [[ ":$PATH:" == *":$HOME/.local/bin:"* ]]; then
        echo -e "${FLEX_YELLOW}提示: 请手动从 ~/.bashrc 中移除 ~/.local/bin${FLEX_NC}"
    fi
else
    echo -e "${FLEX_GREEN}保留用户配置和缓存${FLEX_NC}"
fi

echo -e "\n${FLEX_GREEN}FlexTools 卸载完成!${FLEX_NC}"
echo -e "${FLEX_YELLOW}感谢您使用 FlexTools${FLEX_NC}"
