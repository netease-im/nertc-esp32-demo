#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;
    const uint8_t *start;
    const uint8_t *end;
    uint32_t size;
    uint8_t *psram;
} lz4_res_t;

#define LZ4_RES_END {NULL, NULL, 0, NULL}

extern lz4_res_t lz4_res_list[];
extern const int       lz4_res_count;

const char* lz4_get_gif_name_get_by_name(const char* emotion);

#ifdef __cplusplus
}
#endif