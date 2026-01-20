#ifndef __NERTC_SDK_H__
#define __NERTC_SDK_H__

#include <stdint.h>

#include "nertc_sdk_deprecated.h"
#include "nertc_sdk_error.h"
#include "nertc_sdk_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NERTC_SDK_API __attribute__((visibility("default")))

/**
 * @brief 获取 SDK 版本号
 * @return SDK 版本号
 */
NERTC_SDK_API const char* nertc_get_version(void);

/**
 * @brief 创建引擎实例,该方法是整个SDK调用的第一个方法
 * @param cfg SDK全局配置，详见: {@link nertc_sdk_configuration_t}
 * @return 引擎实例
 */
NERTC_SDK_API nertc_sdk_engine_t nertc_create_engine_with_config(const nertc_sdk_configuration_t* cfg);

/**
 * @brief 销毁引擎实例
 * @param engine 通过 nertc_create_engine 创建的引擎实例
 */
NERTC_SDK_API void nertc_destroy_engine(nertc_sdk_engine_t engine);

/**
 * @brief 初始化引擎实例
 * @note  创建引擎实例之后调用的第一个方法，仅能被初始化一次
 * @param engine 通过 nertc_create_engine 创建且未被初始化的引擎实例
 * @param cfg 引擎实例配置，详见: {@link nertc_sdk_engine_config_t}
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_init_engine(nertc_sdk_engine_t engine, nertc_sdk_engine_config_t* cfg);

/**
 * @brief 加入房间
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param channel_name 房间名
 * @param uid 用户id
 * @param token 动态密钥，用于对加入房间用户进行鉴权验证 <br>
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_join(nertc_sdk_engine_t engine, const char* channel_name, const char* token, uint64_t uid);

/**
 * @brief 离开房间
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_leave(nertc_sdk_engine_t engine);

/**
 * @brief 推送外部音频辅流数据帧。
 * @note
 * - 该方法需要在加入房间后调用。
 * - 数据帧时长需要匹配 on_join 回调中的 nertc_sdk_recommended_config_t 结构体成员变量 recommended_audio_config_t
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param stream_type 流类型
 * @param audio_frame 音频帧
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_push_audio_frame(nertc_sdk_engine_t engine,
                                         nertc_sdk_media_stream_e stream_type,
                                         nertc_sdk_audio_frame_t* audio_frame);
/**
 * @brief 推送外部音频编码帧。
 * @note
 * - 通过此接口可以实现通过音频通道推送外部音频编码后的数据。
 * - 该方法需要在加入房间后调用。
 * - 目前仅支持传输 OPUS 格式的音频数据。
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param stream_type 流类型
 * @param audio_config 音频配置
 * @param audio_encoded_frame 音频编码帧
 * @param audio_rms_level 音频数据音量标记，有效值[0,100]，用于后台 ASL 选路时参考。默认100
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_push_audio_encoded_frame(nertc_sdk_engine_t engine,
                                                 nertc_sdk_media_stream_e stream_type,
                                                 nertc_sdk_audio_config_t audio_config,
                                                 uint8_t audio_rms_level,
                                                 nertc_sdk_audio_encoded_frame_t* audio_encoded_frame);
/**
 * @brief 推送外部音频参考帧。
 * @note
 * - 该方法需要在加入房间后调用。
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param stream_type 流类型
 * @param audio_encoded_frame 解码前的音频编码帧
 * @param audio_frame 解码后的音频帧
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_push_audio_reference_frame(nertc_sdk_engine_t engine,
                                                   nertc_sdk_media_stream_e stream_type,
                                                   nertc_sdk_audio_encoded_frame_t* audio_encoded_frame,
                                                   nertc_sdk_audio_frame_t* audio_frame);
/**
 * @brief 开启字幕
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param config 字幕对应的配置
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_start_asr_caption(nertc_sdk_engine_t engine, nertc_sdk_asr_caption_config_t* config);

/**
 * @brief 停止字幕
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_stop_asr_caption(nertc_sdk_engine_t engine);

/**
 * @brief 开始AI服务
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param config AI服务对应的配置
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_start_ai_with_config(nertc_sdk_engine_t engine, nertc_sdk_start_ai_config_t* config);

/**
 * @brief 停止AI服务
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_stop_ai(nertc_sdk_engine_t engine);

/**
 * @brief 手动打断AI，打电话过程中可以挂断电话
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_ai_manual_interrupt(nertc_sdk_engine_t engine);

/**
 * @brief 手动开启音频识别（按键模式专用）
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_ai_manual_start_listen(nertc_sdk_engine_t engine);

/**
 * @brief 手动关闭音频识别（按键模式专用）
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_ai_manual_stop_listen(nertc_sdk_engine_t engine);

/**
 * @brief AI 文本请求
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param text 文本
 * @param interrupt_mode 打断模式：<br>
 *         - 1：高优先级。传入信息直接打断交互，进行处理。 <br>
 *         - 2：中优先级。等待当前交互结束后，进行处理。 <br>
 *         - 3：低优先级。如当前正在发生交互，直接丢弃。 <br>
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 * */
NERTC_SDK_API int nertc_ai_llm_prompt(nertc_sdk_engine_t engine, const char* text, int interrupt_mode);

/**
 * @brief AI 图片识别请求
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param request AI大模型请求
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 * */
NERTC_SDK_API int nertc_ai_llm_image(nertc_sdk_engine_t engine, nertc_sdk_ai_llm_request_t* request);

/**
 * @brief 外部文本转语音
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param text 外部文本
 * @param interrupt_mode 打断模式：<br>
 *         - 1：高优先级。传入信息直接打断交互，进行处理。 <br>
 *         - 2：中优先级。等待当前交互结束后，进行处理。 <br>
 *         - 3：低优先级。如当前正在发生交互，直接丢弃。 <br>
 * @param add_context 是否添加到聊天上下文中
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 * */
NERTC_SDK_API int nertc_ai_external_tts(nertc_sdk_engine_t engine, const char* text, int interrupt_mode, bool add_context);

/**
 * @brief 回复 mcp tool call 的结果
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @param result：mcp tool call 的详细结果，类型详见: {@link nertc_sdk_mcp_tool_result_t}
 * @return 方法调用结果：<br>
 *              -  0：成功 <br>
 *              - 非0：失败 <br>
 * 
 */
NERTC_SDK_API int nertc_ai_reply_mcp_tool_call(nertc_sdk_engine_t engine, nertc_sdk_mcp_tool_result_t* result);

/**
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *                                                  结构体默认初始化函数
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
NERTC_SDK_API void nertc_sdk_configuration_init(nertc_sdk_configuration_t* cfg);

NERTC_SDK_API void nertc_sdk_engine_config_init(nertc_sdk_engine_config_t* cfg);

NERTC_SDK_API void nertc_sdk_licence_config_init(nertc_sdk_licence_config_t* cfg);

NERTC_SDK_API void nertc_sdk_audio_config_init(nertc_sdk_audio_config_t* cfg);

NERTC_SDK_API void nertc_sdk_log_config_init(nertc_sdk_log_config_t* cfg);

NERTC_SDK_API void nertc_sdk_optional_configuration_init(nertc_sdk_optional_configuration_t* cfg);

NERTC_SDK_API void nertc_sdk_user_info_init(nertc_sdk_user_info_t* info);

NERTC_SDK_API void nertc_sdk_recommended_configuration_init(nertc_sdk_recommended_config_t* cfg);

NERTC_SDK_API void nertc_sdk_audio_frame_init(nertc_sdk_audio_frame_t* frame);

NERTC_SDK_API void nertc_sdk_audio_encoded_frame_init(nertc_sdk_audio_encoded_frame_t* frame);

NERTC_SDK_API void nertc_sdk_start_ai_config_init(nertc_sdk_start_ai_config_t* cfg);

NERTC_SDK_API void nertc_sdk_asr_caption_config_init(nertc_sdk_asr_caption_config_t* cfg);

NERTC_SDK_API void nertc_sdk_asr_caption_result_init(nertc_sdk_asr_caption_result_t* result);

NERTC_SDK_API void nertc_sdk_ai_data_result_init(nertc_sdk_ai_data_result_t* result);

NERTC_SDK_API void nertc_sdk_engine_feature_config_init(nertc_sdk_engine_feature_config_t* config);

NERTC_SDK_API void nertc_sdk_mcp_tool_result_init(nertc_sdk_mcp_tool_result_t* result);

#ifdef __cplusplus
}
#endif
#endif
