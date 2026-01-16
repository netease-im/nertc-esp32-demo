#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function
import os
import sys
import argparse
import subprocess

# 默认串口设备
DEFAULT_PORT = "/dev/ttyUSB0"

def run_command(cmd, error_msg):
    """运行命令并检查结果"""
    try:
        print("执行命令: " + " ".join(cmd))
        result = subprocess.call(cmd)
        if result != 0:
            print(error_msg)
            sys.exit(1)
    except OSError as e:
        print("错误: 无法执行命令 - " + str(e))
        sys.exit(1)

def main():
    # 解析命令行参数
    parser = argparse.ArgumentParser(description='ESP32刷写脚本')
    parser.add_argument('-p', '--port', default=DEFAULT_PORT,
                       help='串口设备 (默认: {})'.format(DEFAULT_PORT))
    args = parser.parse_args()
    
    port = args.port
    
    # 检查串口设备是否存在
    if not os.path.exists(port):
        print("错误: 串口设备 {} 不存在!".format(port))
        sys.exit(1)
    
    print("使用串口: {}".format(port))
    print("波特率: 115200")
    
    # 第一步：刷写主固件
    print("第一步：刷写主固件...")
    run_command(["idf.py", "-p", port, "-b", "115200", "flash"], 
                "错误: 主固件刷写失败")
    
    # 第二步：构建blufi_app
    print("第二步：构建blufi_app...")
    blufi_dir = "third_party/blufi_app"
    if not os.path.exists(blufi_dir):
        print("错误: 目录 {} 不存在".format(blufi_dir))
        sys.exit(1)
    
    original_dir = os.getcwd()
    os.chdir(blufi_dir)
    run_command(["idf.py", "build"], "错误: 构建失败")
    os.chdir(original_dir)
    
    # 第三步：刷写blufi_app固件到指定地址
    print("第三步：刷写blufi_app到0xE20000...")
    blufi_bin = os.path.join(blufi_dir, "build", "blufi_app.bin")
    if not os.path.exists(blufi_bin):
        print("错误: 文件 {} 不存在".format(blufi_bin))
        sys.exit(1)
    
    run_command([
        "esptool.py", "--chip", "esp32s3",
        "--port", port,
        "write_flash", "0xE20000", blufi_bin
    ], "错误: blufi_app刷写失败")
    
    print("所有操作完成!")

if __name__ == "__main__":
    main()