import os
import subprocess

# 定义targets数组
targets = ["doit-ai-speaker", "lichuang-dev", "bread-compact-wifi"]

# 清空releases目录下的所有文件
releases_dir = "releases"
if os.path.exists(releases_dir):
    for file in os.listdir(releases_dir):
        file_path = os.path.join(releases_dir, file)
        try:
            if os.path.isfile(file_path):
                os.unlink(file_path)
        except Exception as e:
            print(f"Error deleting {file_path}: {e}")
else:
    print(f"Directory {releases_dir} does not exist.")

# 遍历targets数组并执行命令
for target in targets:
    command = ["python3", "scripts/release.py", target]
    try:
        subprocess.run(command, check=True)
        print(f"Work done for target: {target}!")
    except subprocess.CalledProcessError as e:
        print(f"Error executing command for target {target}: {e}")