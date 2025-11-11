#!/usr/bin/env python2
# -*- coding: utf-8 -*-
# gif2rgb565_lz4_py2.py  —— Python 2.7 版
import lz4.block
import struct
import argparse
import os
from PIL import Image

def get_gif_fps(gif_path):
    """从 GIF 文件中读取实际的 FPS"""
    im = Image.open(gif_path)
    delay_ms = im.info.get('duration', 100)
    if delay_ms <= 0:
        delay_ms = 100
    fps = 1000.0 / delay_ms
    im.close()
    return fps

def gif_to_lz4(gif_path, fps=None, out_dir=None):
    """把 GIF 逐帧解码成 RGB565 → LZ4 压缩 → 单文件输出（带 GIFL 头）"""
    if out_dir is None:
        out_dir = os.path.dirname(gif_path)
    base_name = os.path.splitext(os.path.basename(gif_path))[0]
    out_file = os.path.join(out_dir, base_name + '.lz4')

    if fps is None:
        fps = get_gif_fps(gif_path)
    fps_int = int(round(fps))

    frames = []
    frame_delays = []

    im = Image.open(gif_path)
    w, h = im.size
    for idx in range(im.n_frames):
        im.seek(idx)
        delay_ms = im.info.get('duration', 100)
        if delay_ms <= 0:
            delay_ms = 100
        frame_delays.append(delay_ms)

        rgb = im.convert('RGB')
        r, g, b = rgb.split()
        r = Image.eval(r, lambda x: x >> 3)
        g = Image.eval(g, lambda x: x >> 2)
        b = Image.eval(b, lambda x: x >> 3)

        rgb565 = bytearray()
        for px in rgb.getdata():
            ri = px[0] >> 3
            gi = px[1] >> 2
            bi = px[2] >> 3
            rgb565.extend(struct.pack('<H', (ri << 11) | (gi << 5) | bi))

        comp = lz4.block.compress(
            bytes(rgb565),
            compression=0,
            store_size=False
        )
        print('comp_len={} rgb565_len={}'.format(len(comp), len(rgb565)))
        frames.append(comp)
    im.close()

    # 写文件：16 字节头 + 帧长度表 + 帧数据
    with open(out_file, 'wb') as f:
        hdr = struct.pack('<4sHHBBHI', b'GIFL', w, h, fps_int, 0, len(frames), 16)
        f.write(hdr)
        for data in frames:
            f.write(struct.pack('<I', len(data)))
            f.write(data)

    print('[OK] {} -> {}  {}x{}  {} frames  {:.2f} fps (from GIF)'.format(
        gif_path, out_file, w, h, len(frames), fps))
    return out_file

def main():
    parser = argparse.ArgumentParser(description='Batch convert GIF to RGB565+LZ4')
    parser.add_argument('path', nargs='*', help='GIF files or folders')
    parser.add_argument('-o', '--output', help='output folder')
    parser.add_argument('-f', '--fps', type=float, help='target fps (overrides GIF fps)')
    args = parser.parse_args()

    gif_list = []
    for p in args.path:
        if os.path.isdir(p):
            for fn in os.listdir(p):
                if fn.lower().endswith('.gif'):
                    gif_list.append(os.path.join(p, fn))
        else:
            gif_list.append(p)
    if not gif_list:
        parser.error('No GIF files found')

    for g in gif_list:
        gif_to_lz4(g, args.fps, args.output)

if __name__ == '__main__':
    main()