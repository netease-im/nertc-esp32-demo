import argparse
import shutil, os
import subprocess
import csv
import sys

# 默认分区表路径
DEFAULT_PARTITION_TABLE = "partitions/v2/16m.csv"
DEFAULT_C3_PARTITION_TABLE = "partitions/v2/16m_c3.csv"
DEFAULT_TARGET = "esp32-s3"  # 默认芯片类型

def parse_partition_table(partition_table_path):
    """解析分区表，获取 custom 分区的 offset 和 size"""
    try:
        with open(partition_table_path, "r") as csvfile:
            reader = csv.reader(csvfile)
            for row in reader:
                if len(row) >= 5 and row[0].strip() == "custom":
                    offset = int(row[3].strip(), 16)  # 第 4 列为 offset
                    size = int(row[4].strip(), 16)    # 第 5 列为 size
                    return offset, size
    except Exception as e:
        print(f"Error parsing partition table: {e}")
        sys.exit(1)
    print(f"Error: Could not find 'custom' partition in {partition_table_path}")
    sys.exit(1)

def build_bin(size, output_path, local_config_path, target):
    """生成 bin 文件"""
    if not os.path.exists(local_config_path):
        print(f"Error: Directory '{local_config_path}' does not exist.")
        sys.exit(1)

    command = f"python create_local_config/spiffsgen.py {size} {local_config_path} {output_path}"
    print(f"Running build command for target {target}: {command}")
    try:
        subprocess.run(command, shell=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error: Failed to build bin file. {e}")
        sys.exit(1)

def flash_bin(port, write_at, bin_path, target):
    """烧录 bin 文件到 flash"""
    command = f"python -m esptool --chip {target} write_flash {write_at} {bin_path}"
    if port:
        command = f"python -m esptool --chip {target} -p {port} write_flash {write_at} {bin_path}"
    print(f"Running flash command for target {target}: {command}")
    try:
        subprocess.run(command, shell=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error: Failed to flash bin file. {e}")
        sys.exit(1)

def flash_blufi(port, target):
    """烧录蓝牙配网固件"""
    app_name="blufi_app.bin"
    if target == "esp32-c3":
        app_name="blufi_app_c3.bin"
    command = f"python -m esptool --chip {target} -p {port} write_flash 0x520000 third_party/blufi_app/bin/{app_name}"
    print(f"Running blufi flash command: {command}")
    try:
        subprocess.run(command, shell=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error: Failed to flash blufi firmware. {e}")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="Config script for building and flashing.")
    parser.add_argument("-p", "--port", help="Specify the serial port (e.g., /dev/ttyUSB0).")
    parser.add_argument("--build", action="store_true", help="Build the bin file.")
    parser.add_argument("--flash", action="store_true", help="Flash the bin file to the device.")
    parser.add_argument("-o", "--output", default="config.bin", help="Output path for the bin file.")
    parser.add_argument("-i", "--input", default="", help="Input directory for building or bin file for flashing.")
    parser.add_argument("--size", help="Specify the size for the bin file (e.g., 0x2000).")
    parser.add_argument("--write-at", help="Specify the flash write address (e.g., 0x36100).")
    parser.add_argument("--target", default=DEFAULT_TARGET, help="Specify the target chip (e.g., esp32, esp32-s3, esp32-c3). Default is esp32-s3.")
    parser.add_argument("--blufi", action="store_true", help="Flash the blufi firmware for Bluetooth provisioning.")
    args = parser.parse_args()

    # 根据target选择分区表文件
    partition_table_path = DEFAULT_C3_PARTITION_TABLE if args.target == "esp32-c3" else DEFAULT_PARTITION_TABLE

    # 如果没有指定 --size 或 --write-at，从分区表中解析
    size = args.size
    write_at = args.write_at
    if not size or not write_at:
        offset, partition_size = parse_partition_table(partition_table_path)
        if not size:
            size = hex(partition_size)
        if not write_at:
            write_at = hex(offset)

    # 执行 --build
    if args.build:
        if not args.input:
            # 如果没有指定输入目录，根据芯片类型复制对应的配置文件
            if not os.path.exists("local_config"):
                os.makedirs("local_config")

            if args.target == "esp32-s3":
                shutil.copy("create_local_config/config.json.s3", "local_config/config.json")
            else:  # esp32-c3 or other targets
                shutil.copy("create_local_config/config.json.c3", "local_config/config.json")

            args.input = "local_config"
        build_bin(size, args.output, args.input, args.target)

    # 执行 --flash
    if args.flash:
        if not args.port:
            print("Error: --flash requires -p/--port to specify the serial port.")
            sys.exit(1)
        flash_bin(args.port, write_at, args.output, args.target)

    # 执行 --blufi
    if args.blufi:
        if not args.port:
            print("Error: --blufi requires -p/--port to specify the serial port.")
            sys.exit(1)
        flash_blufi(args.port, args.target)

if __name__ == "__main__":
    main()
    command = f"git checkout local_config/config.json"
    print(f"Running git checkout command: {command}")
    try:
        subprocess.run(command, shell=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error: Failed to checkout config file. {e}")
        sys.exit(1)
