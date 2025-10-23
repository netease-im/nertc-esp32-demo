#!/usr/bin/env python3 all_to_gif.py ./
import re, pathlib, sys

src_dir = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else '.')
for c_file in src_dir.glob('*.c'):
    txt = c_file.read_text(errors='ignore')
    hex_str = re.findall(r'0x[0-9a-fA-F]{2}', txt)
    if not hex_str:
        print(f'⚠️  跳过 {c_file.name}：未找到十六进制数组')
        continue
    data = bytes(int(x, 16) for x in hex_str)
    gif_file = c_file.with_suffix('.gif')
    gif_file.write_bytes(data)
    print(f'✅ 已生成 {gif_file.name}')