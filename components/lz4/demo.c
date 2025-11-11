/* demo.c - 解析带 GIFL 头的 .lz4 文件，解压第 0 帧并输出可播放文件 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "lz4.h"

typedef struct {
    char magic[4];        // 'G','I','F','L'
    uint16_t w, h;
    uint8_t fps, reserved;
    uint16_t frames;
    uint32_t dataOffset;
} gif_hdr_t;

// 小端序读取函数
static uint16_t read_le16(const uint8_t *data) {
    return data[0] | (data[1] << 8);
}

static uint32_t read_le32(const uint8_t *data) {
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

// RGB565转RGB888
static void rgb565_to_rgb888(const uint8_t *rgb565, uint8_t *rgb888, int pixel_count) {
    for (int i = 0; i < pixel_count; i++) {
        uint16_t pixel = rgb565[i*2] | (rgb565[i*2+1] << 8);
        
        // 提取RGB565分量
        uint8_t r = (pixel >> 11) & 0x1F;  // 5位
        uint8_t g = (pixel >> 5)  & 0x3F;  // 6位  
        uint8_t b = pixel & 0x1F;          // 5位
        
        // 扩展到8位
        rgb888[i*3]   = (r << 3) | (r >> 2);   // 5->8位
        rgb888[i*3+1] = (g << 2) | (g >> 4);   // 6->8位
        rgb888[i*3+2] = (b << 3) | (b >> 2);   // 5->8位
    }
}

// 保存为PPM文件（简单格式，ffplay可直接播放）
static int save_ppm(const char *filename, const uint8_t *rgb888, int width, int height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen ppm");
        return -1;
    }
    
    // PPM头：P6表示二进制RGB格式
    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    fwrite(rgb888, width * height * 3, 1, fp);
    fclose(fp);
    return 0;
}

// 保存为RAW RGB文件
static int save_raw_rgb(const char *filename, const uint8_t *rgb888, int width, int height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen raw");
        return -1;
    }
    
    fwrite(rgb888, width * height * 3, 1, fp);
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("usage: %s <file.lz4> [output_prefix]\n", argv[0]);
        printf("Example: %s happy.lz4 frame\n", argv[0]);
        return 1;
    }

    const char *output_prefix = (argc > 2) ? argv[2] : "output";

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) { perror("fopen"); return 1; }

    // 读取16字节头数据
    uint8_t header_buf[16];
    if (fread(header_buf, 16, 1, fp) != 1) {
        printf("read header failed\n");
        fclose(fp);
        return 1;
    }

    // 解析各个字段（小端序）
    gif_hdr_t hd;
    memcpy(hd.magic, header_buf, 4);
    hd.w = read_le16(header_buf + 4);
    hd.h = read_le16(header_buf + 6);
    hd.fps = header_buf[8];
    hd.reserved = header_buf[9];
    hd.frames = read_le16(header_buf + 10);
    hd.dataOffset = read_le32(header_buf + 12);

    // 检查magic
    if (memcmp(hd.magic, "GIFL", 4) != 0) {
        printf("bad GIFL header: got %.4s\n", hd.magic);
        fclose(fp);
        return 1;
    }

    printf("GIFL %ux%u fps=%u frames=%u dataOffset=%u\n", 
           hd.w, hd.h, hd.fps, hd.frames, hd.dataOffset);

    // 为所有帧分配内存
    uint32_t frame_size = hd.w * hd.h * 2;
    uint32_t rgb888_size = hd.w * hd.h * 3;
    
    uint8_t *rgb888_buffer = malloc(rgb888_size);
    if (!rgb888_buffer) {
        printf("oom for rgb888 buffer\n");
        fclose(fp);
        return 1;
    }

    // 处理每一帧
    for (int frame_idx = 0; frame_idx < hd.frames; frame_idx++) {
        // 读取压缩长度
        uint32_t cmpLen;
        uint8_t len_buf[4];
        if (fread(len_buf, 4, 1, fp) != 1) {
            printf("read cmpLen for frame %d failed\n", frame_idx);
            break;
        }
        cmpLen = read_le32(len_buf);

        uint8_t *cmpBuf = malloc(cmpLen);
        if (!cmpBuf) {
            printf("oom for compressed data\n");
            break;
        }
        
        if (fread(cmpBuf, cmpLen, 1, fp) != 1) {
            printf("read cmp data for frame %d failed\n", frame_idx);
            free(cmpBuf);
            break;
        }

        uint8_t *rgb565 = malloc(frame_size);
        if (!rgb565) {
            free(cmpBuf);
            printf("oom for rgb565 buffer\n");
            break;
        }

        /* 解压 */
        int decBytes = LZ4_decompress_safe((const char *)cmpBuf,
                                          (char *)rgb565,
                                          cmpLen,
                                          frame_size);
        free(cmpBuf);
        
        if (decBytes < 0) {
            printf("LZ4_decompress_safe error %d for frame %d\n", decBytes, frame_idx);
            free(rgb565);
            break;
        }

        printf("Frame %d: decompress OK %u -> %d bytes\n", frame_idx, cmpLen, decBytes);

        printf("RGB565:");
        uint16_t *px = (uint16_t *)rgb565;
        for (int i = 0; i < 10000; i+=100) {
            printf("%04X ", px[i]);
        }
        printf("\n");

        // 转换为RGB888
        rgb565_to_rgb888(rgb565, rgb888_buffer, hd.w * hd.h);
        free(rgb565);

        printf("RGB888:");
        px = (uint16_t *)rgb888_buffer;
        for (int i = 0; i < 10000; i+=100) {
            printf("%04X ", px[i]);
        }
        printf("\n");

        // 保存为PPM文件
        char ppm_filename[256];
        snprintf(ppm_filename, sizeof(ppm_filename), "%s_%03d.ppm", output_prefix, frame_idx);
        if (save_ppm(ppm_filename, rgb888_buffer, hd.w, hd.h) == 0) {
            printf("Saved: %s\n", ppm_filename);
        }

        // 同时保存为RAW文件（可选）
        char raw_filename[256];
        snprintf(raw_filename, sizeof(raw_filename), "%s_%03d.raw", output_prefix, frame_idx);
        save_raw_rgb(raw_filename, rgb888_buffer, hd.w, hd.h);
    }

    fclose(fp);
    free(rgb888_buffer);

    printf("\n播放命令:\n");
    printf("ffplay -f rawvideo -pixel_format rgb24 -video_size %dx%d %s_000.raw\n", 
           hd.w, hd.h, output_prefix);
    printf("或者播放PPM序列: ffmpeg -i %s_%%03d.ppm output.gif\n", output_prefix);

    return 0;
}