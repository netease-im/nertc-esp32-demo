/* lz4_auto.c  */
#include "lz4_auto.h"
#include <stddef.h>      // NULL
#include <stdint.h>
#include <string.h>

/* 外部符号：只当指针用 */

#define DEF_X(name)  \
extern const uint8_t _binary_##name##_lz4_start;\
extern const uint8_t _binary_##name##_lz4_end;

#define X(name)  { #name, \
                   (const uint8_t *)&_binary_##name##_lz4_start, \
                   (const uint8_t *)&_binary_##name##_lz4_end, \
                   0,      /* size 先填 0 */ \
                   NULL },

typedef struct {
    const char* emotion;
    const char* name;
} emotion_name_map_t;

#if defined(CONFIG_USE_ANIM_EMOJI_NERTC1) || defined(CONFIG_USE_ANIM_EMOJI_NERTC2_160)
DEF_X(angry)
DEF_X(confused)
DEF_X(crying)
DEF_X(embarrassed)
DEF_X(happy)
DEF_X(mute)
DEF_X(loving)
DEF_X(sleepy)
DEF_X(surprised)
DEF_X(thinking)
DEF_X(wifi)
DEF_X(winking)

lz4_res_t lz4_res_list[] = { 
    X(winking)  
    X(angry)
    X(confused)
    X(crying)
    X(embarrassed)
    X(happy)
    X(mute)
    X(loving)
    X(sleepy)
    X(surprised)
    X(thinking)
    X(wifi)
    { NULL, NULL, NULL, 0, NULL }      /* 结束标记 */
};
static const emotion_name_map_t emotion_maps[] = {
    {"neutral",     "winking"},      // 眨眼（默认，正式符号）
    {"happy",       "happy"},        // 笑眼
    {"laughing",    "happy"},        // 笑眼
    {"funny",       "happy"},        // 笑眼
    {"sad",         "crying"},       // 想哭
    {"angry",       "angry"},        // 怒火
    {"crying",      "crying"},       // 想哭
    {"loving",      "loving"},       // 喜欢
    {"embarrassed", "embarrassed"},  // 委屈
    {"surprised",   "surprised"},    // 惊讶
    {"shocked",     "surprised"},    // 惊讶
    {"thinking",    "thinking"},     // 思考
    {"winking",     "winking"},      // 眨眼
    {"cool",        "happy"},        // 中性 → 笑眼
    {"relaxed",     "happy"},        // 中性 → 笑眼
    {"delicious",   "loving"},       // 喜欢
    {"kissy",       "loving"},       // 喜欢
    {"confident",   "happy"},        // 中性 → 笑眼
    {"sleepy",      "sleepy"},       // 困
    {"silly",       "happy"},        // 笑眼
    {"confused",    "confused"},     // 晕
    {"wifi",        "wifi"},         // wifi 联网
    {"mute",        "mute"},         // 静音
    {NULL, NULL}                    // 结束标记
};
#elif defined(CONFIG_USE_ANIM_EMOJI_NERTC2)
DEF_X(angry)
DEF_X(confused)
DEF_X(crying)
DEF_X(embarrassed)
DEF_X(happy)
DEF_X(mute)
DEF_X(loving)
DEF_X(sleepy)
DEF_X(surprised)
DEF_X(thinking)
DEF_X(wifi)
DEF_X(winking)
DEF_X(call)

lz4_res_t lz4_res_list[] = { 
    X(winking)  
    X(angry)
    X(call)
    X(confused)
    X(crying)
    X(embarrassed)
    X(happy)
    X(mute)
    X(loving)
    X(sleepy)
    X(surprised)
    X(thinking)
    X(wifi)
    { NULL, NULL, NULL, 0, NULL }      /* 结束标记 */
};
static const emotion_name_map_t emotion_maps[] = {
    {"neutral",     "winking"},      // 眨眼（默认，正式符号）
    {"happy",       "happy"},        // 笑眼
    {"laughing",    "happy"},        // 笑眼
    {"funny",       "happy"},        // 笑眼
    {"sad",         "crying"},       // 想哭
    {"angry",       "angry"},        // 怒火
    {"crying",      "crying"},       // 想哭
    {"loving",      "loving"},       // 喜欢
    {"embarrassed", "embarrassed"},  // 委屈
    {"surprised",   "surprised"},    // 惊讶
    {"shocked",     "surprised"},    // 惊讶
    {"thinking",    "thinking"},     // 思考
    {"winking",     "winking"},      // 眨眼
    {"cool",        "happy"},        // 中性 → 笑眼
    {"relaxed",     "happy"},        // 中性 → 笑眼
    {"delicious",   "loving"},       // 喜欢
    {"kissy",       "loving"},       // 喜欢
    {"confident",   "happy"},        // 中性 → 笑眼
    {"sleepy",      "sleepy"},       // 困
    {"silly",       "happy"},        // 笑眼
    {"confused",    "confused"},     // 晕
    {"wifi",        "wifi"},         // wifi 联网
    {"mute",        "mute"},         // 静音
    {"call",        "call"},         // 打电话
    {NULL, NULL}                    // 结束标记
};
#elif defined(CONFIG_USE_ANIM_EMOJI_NERTC3) || defined(CONFIG_USE_ANIM_EMOJI_NERTC3_160)
DEF_X(angry)
DEF_X(confused)
DEF_X(crying)
DEF_X(embarrassed)
DEF_X(happy)
DEF_X(mute)
DEF_X(loving)
DEF_X(sleepy)
DEF_X(surprised)
DEF_X(thinking)
DEF_X(wifi)
DEF_X(winking)
DEF_X(star)

lz4_res_t lz4_res_list[] = {   
    X(winking)
    X(angry)
    X(confused)
    X(crying)
    X(embarrassed)
    X(happy)
    X(mute)
    X(loving)
    X(sleepy)
    X(surprised)
    X(thinking)
    X(wifi)
    X(star)
    { NULL, NULL, NULL, 0, NULL }      /* 结束标记 */
};
static const emotion_name_map_t emotion_maps[] = {
    {"neutral",     "winking"},      // 眨眼（默认，正式符号）
    {"happy",       "star"},        // 笑眼
    {"laughing",    "star"},        // 笑眼
    {"funny",       "star"},        // 笑眼
    {"sad",         "crying"},       // 想哭
    {"angry",       "angry"},        // 怒火
    {"crying",      "crying"},       // 想哭
    {"loving",      "loving"},       // 喜欢
    {"embarrassed", "embarrassed"},  // 委屈
    {"surprised",   "surprised"},    // 惊讶
    {"shocked",     "surprised"},    // 惊讶
    {"thinking",    "thinking"},     // 思考
    {"winking",     "winking"},         // 星星眼
    {"cool",        "star"},         // 星星眼
    {"relaxed",     "happy"},        // 中性 → 笑眼
    {"delicious",   "loving"},       // 喜欢
    {"kissy",       "loving"},       // 喜欢
    {"confident",   "happy"},        // 中性 → 笑眼
    {"sleepy",      "sleepy"},       // 困
    {"silly",       "happy"},        // 笑眼
    {"confused",    "confused"},     // 晕
    {"wifi",        "wifi"},         // wifi 联网
    {"mute",        "mute"},         // 静音
    {NULL, NULL}                    // 结束标记
};

#elif defined(CONFIG_USE_ANIM_EMOJI_OTTO)
DEF_X(staticstate)
DEF_X(sad)
DEF_X(happy)
DEF_X(scare)
DEF_X(buxue)
DEF_X(anger)

lz4_res_t lz4_res_list[] = {   
    X(staticstate)
    X(sad)
    X(happy)
    X(scare)
    X(buxue)
    X(anger)
    { NULL, NULL, NULL, 0, NULL }      /* 结束标记 */
};

static const emotion_name_map_t emotion_maps[] = {
    {"neutral",     "staticstate"},
    {"happy",       "happy"},
    {"laughing",    "happy"},
    {"funny",       "happy"},
    {"sad",         "sad"},
    {"angry",       "anger"},
    {"crying",      "sad"},
    {"loving",      "happy"},
    {"embarrassed", "buxue"},
    {"surprised",   "scare"},
    {"shocked",     "scare"},
    {"thinking",    "buxue"},
    {"winking",     "happy"},
    {"cool",        "happy"},
    {"relaxed",     "staticstate"},
    {"delicious",   "happy"},
    {"kissy",       "happy"},
    {"confident",   "happy"},
    {"sleepy",      "staticstate"},
    {"silly",       "happy"},
    {"confused",    "buxue"},
    {NULL, NULL}   // 结束标记
};
#else
lz4_res_t lz4_res_list[] = {   
    { NULL, NULL, NULL, 0, NULL }      /* 结束标记 */
};
static const emotion_name_map_t emotion_maps[] = {
    {NULL, NULL}   // 结束标记
};
#endif

const int lz4_res_count = sizeof(lz4_res_list)/sizeof(lz4_res_list[0]) - 1;

/* 启动时一次性计算 size */
static void __attribute__((constructor)) lz4_fill_size(void)
{
    for (int i = 0; i < lz4_res_count; ++i)
        lz4_res_list[i].size = lz4_res_list[i].end
                             - lz4_res_list[i].start;
}

const char* lz4_get_gif_name_get_by_name(const char* emotion)
{
    if (emotion == NULL) {
        return "";
    }

    for (int i = 0; emotion_maps[i].emotion != NULL; i++) {
        if (strcmp(emotion, emotion_maps[i].emotion) == 0) {
            return emotion_maps[i].name;
        }
    }

    return "";
}