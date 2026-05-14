#!/bin/bash

# 登录注册服务 - 一键编译脚本

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "======================================"
echo "  登录注册服务 - 编译"
echo "======================================"
echo ""

# 清理旧的构建
if [ -d "$BUILD_DIR" ]; then
    read -p "是否清理旧的构建文件？(y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "清理构建目录..."
        rm -rf "$BUILD_DIR"
        rm -f "$PROJECT_DIR/server" "$PROJECT_DIR/client" "$PROJECT_DIR/benchmark"
    fi
fi

echo ""
echo "选择编译方式："
echo "1) CMake (推荐)"
echo "2) Makefile"
echo "3) 全部"
read -p "请选择 (1-3): " choice

echo ""
case $choice in
    1)
        echo "使用 CMake 编译..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR" || exit 1
        cmake .. || {
            echo "CMake 配置失败"
            exit 1
        }
        make -j$(nproc) || {
            echo "编译失败"
            exit 1
        }
        cd "$PROJECT_DIR" || exit 1
        echo ""
        echo "✓ 编译完成！"
        echo "可执行文件位于: $BUILD_DIR/"
        ;;
    2)
        echo "使用 Makefile 编译..."
        cd "$PROJECT_DIR" || exit 1
        if [ ! -f "Makefile" ]; then
            echo "错误：未找到 Makefile"
            exit 1
        fi
        make -j$(nproc) || {
            echo "编译失败"
            exit 1
        }
        echo ""
        echo "✓ 编译完成！"
        echo "可执行文件位于: $PROJECT_DIR/"
        ;;
    3)
        echo "编译全部..."
        # CMake
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR" || exit 1
        cmake .. || true
        make -j$(nproc) || true
        cd "$PROJECT_DIR" || exit 1
        # Makefile
        if [ -f "Makefile" ]; then
            make -j$(nproc) || true
        fi
        echo ""
        echo "✓ 编译完成！"
        ;;
    *)
        echo "无效的选择"
        exit 1
        ;;
esac

echo ""
echo "======================================"
echo "  编译完成！"
echo "======================================"
echo ""
echo "使用方法："
echo "  启动服务器: ./start_server.sh"
echo "  启动客户端: ./start_client.sh"
echo ""
