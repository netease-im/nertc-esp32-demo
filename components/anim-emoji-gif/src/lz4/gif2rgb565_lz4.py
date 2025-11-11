#!/usr/bin/env python3
# gif2rgb565_lz4.py  —— 带 16 字节 meta 头
import lz4.block 
import struct
import argparse
from pathlib import Path
from PIL import Image

def get_gif_fps(gif_path: Path) -> float:
    """从 GIF 文件中读取实际的 FPS"""
    with Image.open(gif_path) as im:
        # 获取第一帧的延迟时间（单位：毫秒）
        try:
            delay_ms = im.info.get('duration', 100)  # 默认 100ms
        except:
            delay_ms = 100
        
        # 如果延迟时间为0，使用默认值
        if delay_ms <= 0:
            delay_ms = 100
        
        # 计算 FPS
        fps = 1000.0 / delay_ms
        return fps

def gif_to_lz4(gif_path: Path, fps: float | None = None, out_dir: Path | None = None) -> Path:
    """把 GIF 逐帧解码成 RGB565 → LZ4 压缩 → 单文件输出（带 GIFL 头）"""
    if out_dir is None:
        out_dir = gif_path.parent
    out_file = out_dir / (gif_path.stem + '.lz4')

    # 如果没有提供 FPS，则从 GIF 读取
    if fps is None:
        fps = get_gif_fps(gif_path)
    
    # 确保 FPS 是整数（如果需要）
    fps_int = int(round(fps))

    frames = []
    frame_delays = []  # 存储每帧的延迟时间
    
    with Image.open(gif_path) as im:
        w, h = im.size          # 取宽高
        for idx in range(im.n_frames):
            im.seek(idx)
            
            # 获取当前帧的延迟时间
            try:
                delay_ms = im.info.get('duration', 100)
                if delay_ms <= 0:
                    delay_ms = 100
                frame_delays.append(delay_ms)
            except:
                frame_delays.append(100)
            
            rgb = im.convert('RGB')
            r, g, b = rgb.split()
            r = Image.eval(r, lambda x: x >> 3)
            g = Image.eval(g, lambda x: x >> 2)
            b = Image.eval(b, lambda x: x >> 3)

            rgb565 = bytearray()
            for px in rgb.getdata():        # px = (R, G, B)
                ri = px[0] >> 3
                gi = px[1] >> 2
                bi = px[2] >> 3
                rgb565.extend(struct.pack('<H', (ri << 11) | (gi << 5) | bi))

            comp = lz4.block.compress(
                rgb565, 
                compression=0,  # 快速压缩
                store_size=False  # 不在数据中存储原始大小
            )
            print(f'comp_len={len(comp)} rgb565_len={len(rgb565)}')
            frames.append(comp)

    # 写文件：16 字节头 + 帧长度表 + 帧数据
    with open(out_file, 'wb') as f:
        # 1. meta 头
        hdr = struct.pack('<4sHHBBHI', b'GIFL', w, h, fps_int, 0, len(frames), 16)
        f.write(hdr)
        # 2. 每帧长度(uint32) + 数据
        for data in frames:
            print(f'len={len(data)}')
            f.write(struct.pack('<I', len(data)))
            f.write(data)

    print(f'[OK] {gif_path} -> {out_file}  {w}x{h}  {len(frames)} frames  {fps:.2f} fps (from GIF)')
    return out_file


def main():
    parser = argparse.ArgumentParser(description='Batch convert GIF to RGB565+LZ4')
    parser.add_argument('path', nargs='*', help='GIF files or folders')
    parser.add_argument('-o', '--output', type=Path, help='output folder')
    parser.add_argument('-f', '--fps', type=float, help='target fps (overrides GIF fps)')
    args = parser.parse_args()

    gif_list = []
    for p in args.path:
        path = Path(p)
        if path.is_dir():
            gif_list.extend(path.glob('*.gif'))
        else:
            gif_list.append(path)
    if not gif_list:
        parser.error('No GIF files found')

    for g in gif_list:
        gif_to_lz4(g, args.fps, args.output)


if __name__ == '__main__':
    main()