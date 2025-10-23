/**
 * @file anim_emoji_gif_utils.c
 * @brief GIF表情资源组件辅助函数实现
 */

#include <string.h>

#include "anim_emoji_gif.h"

// 表情映射表
typedef struct {
    const char* name;
    const lv_image_dsc_t* gif;
} emotion_map_t;

#if defined(CONFIG_USE_ANIM_EMOJI_NERTC)

#elif defined(CONFIG_USE_ANIM_EMOJI_OTTO)
// 外部声明的GIF资源
extern const lv_image_dsc_t staticstate;
extern const lv_image_dsc_t sad;
extern const lv_image_dsc_t happy;
extern const lv_image_dsc_t scare;
extern const lv_image_dsc_t buxue;
extern const lv_image_dsc_t anger;

// 表情映射表（21 个表情全部映射到 6 个目标状态）
static const emotion_map_t emotion_maps[] = {
    {"neutral",     &staticstate},
    {"happy",       &happy},
    {"laughing",    &happy},
    {"funny",       &happy},
    {"sad",         &sad},
    {"angry",       &anger},
    {"crying",      &sad},
    {"loving",      &happy},
    {"embarrassed", &buxue},
    {"surprised",   &scare},
    {"shocked",     &scare},
    {"thinking",    &buxue},
    {"winking",     &happy},
    {"cool",        &happy},
    {"relaxed",     &staticstate},
    {"delicious",   &happy},
    {"kissy",       &happy},
    {"confident",   &happy},
    {"sleepy",      &staticstate},
    {"silly",       &happy},
    {"confused",    &buxue},
    {NULL, NULL}   // 结束标记
};
#elif defined(CONFIG_USE_ANIM_EYE_240X240_GIF1) || defined(CONFIG_USE_ANIM_EYE_160X160_GIF1)
extern const lv_image_dsc_t angry;
extern const lv_image_dsc_t confused;
extern const lv_image_dsc_t cool;
extern const lv_image_dsc_t delicious;
extern const lv_image_dsc_t happy;
extern const lv_image_dsc_t love;
extern const lv_image_dsc_t sad;
extern const lv_image_dsc_t sleepy;
extern const lv_image_dsc_t thinking;

static const emotion_map_t emotion_maps[] = {
    {"neutral",     &happy},
    {"happy",       &happy},
    {"laughing",    &happy},
    {"funny",       &happy},
    {"sad",         &sad},
    {"angry",       &angry},
    {"crying",      &sad},
    {"loving",      &love},
    {"embarrassed", &confused},
    {"surprised",   &delicious},
    {"shocked",     &delicious},
    {"thinking",    &thinking},
    {"winking",     &cool},
    {"cool",        &cool},
    {"relaxed",     &happy},
    {"delicious",   &delicious},
    {"kissy",       &love},
    {"confident",   &confused},
    {"sleepy",      &sleepy},
    {"silly",       &delicious},
    {"confused",    &confused},
    {NULL, NULL}               /* 结束标记 */
};
#elif defined(CONFIG_USE_ANIM_EYE_240X240_GIF2) || defined(CONFIG_USE_ANIM_EYE_160X160_GIF2)
extern const lv_image_dsc_t angry;
extern const lv_image_dsc_t confused;
extern const lv_image_dsc_t happy;
extern const lv_image_dsc_t love;
extern const lv_image_dsc_t neutral;
extern const lv_image_dsc_t sleepy;
extern const lv_image_dsc_t thinking;
extern const lv_image_dsc_t winking;

static const emotion_map_t emotion_maps[] = {
    {"neutral",     &neutral},
    {"happy",       &happy},
    {"laughing",    &happy},
    {"funny",       &happy},
    {"sad",         &neutral},
    {"angry",       &angry},
    {"crying",      &neutral},
    {"loving",      &love},
    {"embarrassed", &confused},
    {"surprised",   &confused},
    {"shocked",     &winking},
    {"thinking",    &thinking},
    {"winking",     &winking},
    {"cool",        &winking},
    {"relaxed",     &happy},
    {"delicious",   &winking},
    {"kissy",       &love},
    {"confident",   &confused},
    {"sleepy",      &sleepy},
    {"silly",       &neutral},
    {"confused",    &confused},
    {NULL, NULL}
};
#else
static const emotion_map_t emotion_maps[] = {
    {NULL, NULL}   // 结束标记
};
#endif

const char* anim_emoji_gif_get_version(void) {
    return "1.0.1";
}

const lv_image_dsc_t* anim_emoji_gif_get_by_name(const char* name) {
    if (name == NULL) {
        return NULL;
    }

    for (int i = 0; emotion_maps[i].name != NULL; i++) {
        if (strcmp(emotion_maps[i].name, name) == 0) {
            return emotion_maps[i].gif;
        }
    }

    return NULL;  // 未找到
}