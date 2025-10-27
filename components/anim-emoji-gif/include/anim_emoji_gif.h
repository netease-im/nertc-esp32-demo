/**
 * @file anim_emoji_gif.h
 * @brief GIF表情资源组件
 *
 * 这个头文件声明了GIF表情资源，用于在LVGL显示屏上显示动态表情。
 *
 * 支持的表情：
 * - neutral: 静态状态/中性表情
 * - sad: 悲伤表情
 * - happy: 开心表情
 * - surprised: 惊吓/惊讶表情
 * - confused: 不学/困惑表情
 * - angry: 愤怒表情
 * - laughing: 大笑表情
 * - funny: 搞笑表情
 * - crying: 哭泣表情
 * - loving: 恋爱表情
 * - embarrassed: 尴尬表情
 * - shocked: 震惊表情
 * - thinking: 思考表情
 * - winking: 眨眼表情
 * - cool: 酷炫表情
 * - relaxed: 放松表情
 * - delicious: 美味表情
 * - kissy: 飞吻表情
 * - confident: 自信表情
 * - sleepy: 困倦表情
 * - silly: 调皮表情
 */

#pragma once

#include <libs/gif/lv_gif.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 表情GIF声明
 *
 * 这些GIF资源可以直接用于LVGL的lv_gif组件
 *
 * 使用示例：
 * ```c
 * lv_obj_t* gif = lv_gif_create(parent);
 * lv_image_dsc_t* happy = anim_emoji_gif_get_by_name("happy");
 * lv_gif_set_src(gif, &happy);  // 设置开心表情
 * ```
 */

/**
 * @brief 获取组件版本
 * @return 版本字符串
 */
const char* anim_emoji_gif_get_version(void);

/**
 * @brief 根据名称获取表情资源
 * @param name 表情名称
 * @return 表情资源指针，如果未找到则返回NULL
 */
const lv_image_dsc_t* anim_emoji_gif_get_by_name(const char* name);

#ifdef __cplusplus
}
#endif