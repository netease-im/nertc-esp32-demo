#!/bin/bash

# 默认串口设备
DEFAULT_PORT="/dev/ttyUSB0"
PORT="$DEFAULT_PORT"

# 显示用法说明
usage() {
    echo "用法: $0 [-p 串口设备] [-h]"
    echo "  默认串口: $DEFAULT_PORT"
    echo "  示例: $0 -p /dev/ttyUSB1"
    exit 1
}

# 解析命令行参数
while getopts ":p:h" opt; do
    case $opt in
        p)
            PORT="$OPTARG"
            ;;
        h)
            usage
            ;;
        \?)
            echo "无效选项: -$OPTARG" >&2
            usage
            ;;
        :)
            echo "选项 -$OPTARG 需要参数." >&2
            usage
            ;;
    esac
done

# 检查串口设备是否存在
if [ ! -e "$PORT" ]; then
    echo "错误: 串口设备 $PORT 不存在!"
    exit 1
fi

echo "使用串口: $PORT"
echo "波特率: 115200"

# 第一步：刷写主固件
echo "第一步：刷写主固件..."
idf.py -p "$PORT" -b 115200 flash || { echo "错误: 主固件刷写失败"; exit 1; }

# 第二步：构建blufi_app
echo "第二步：构建blufi_app..."
cd third_party/blufi_app || { echo "错误: 无法进入 third_party/blufi_app 目录"; exit 1; }
idf.py build || { echo "错误: 构建失败"; exit 1; }
cd ../.. || { echo "错误: 无法返回上级目录"; exit 1; }

# 第三步：刷写blufi_app固件到指定地址
echo "第三步：刷写blufi_app到0xE20000..."
esptool.py --chip esp32s3 \
           --port "$PORT" \
           write_flash 0xE20000 \
           third_party/blufi_app/build/blufi_app.bin || { echo "错误: blufi_app刷写失败"; exit 1; }

echo "所有操作完成!"