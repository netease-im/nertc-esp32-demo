#!/usr/bin/env python3
# gif2lvgl.py
import os
import glob
from pathlib import Path

from PIL import Image
import numpy as np
from jinja2 import Template

TPL = Template(r"""#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_{{ name_upper }}
#define LV_ATTRIBUTE_IMG_{{ name_upper }}
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_{{ name_upper }} uint8_t {{ name_lower }}_map[] = {
{{ data }}
};

const lv_img_dsc_t {{ name_lower }} = {
  .header.cf = LV_COLOR_FORMAT_RAW,
  .header.w = {{ w }},
  .header.h = {{ h }},
  .data_size = {{ data_size }},
  .data = {{ name_lower }}_map,
};
""")
def gif_to_c(gif_path: Path):
    """把 .gif 原封不动地塞进 C 数组"""
    name = gif_path.stem
    name_upper = name.upper()
    name_lower = name.lower()

    # 1. 直接按二进制读整个文件
    raw = gif_path.read_bytes()
    data_size = len(raw)

    # 2. 生成 C 数组文本（每行 16 字节）
    hex_lines = [", ".join(f"0x{b:02x}" for b in raw[i:i+16])
                 for i in range(0, len(raw), 16)]
    data_str = ",\n".join("    " + line for line in hex_lines)

    # 3. 像素宽高无法从原始文件直接得知，写 0 或自己填
    w = 0
    h = 0

    # 4. 填充模板
    c_code = TPL.render(
        name_upper=name_upper,
        name_lower=name_lower,
        w=w,
        h=h,
        data_size=data_size,
        data=data_str,
    )

    out_path = gif_path.with_suffix(".c")
    out_path.write_text(c_code, encoding="utf-8")
    print(f"[OK] {gif_path}  ->  {out_path}")

def main():
    for gif in glob.glob("*.gif"):
        gif_to_c(Path(gif))

if __name__ == "__main__":
    main()