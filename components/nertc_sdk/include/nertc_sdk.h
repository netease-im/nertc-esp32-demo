#ifndef _NERTC_SDK_H_
#define _NERTC_SDK_H_

#include "nertc_sdk_event.h"
#include "nertc_sdk_error.h"

#include <stdint.h>

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
 * @param cfg 引擎配置
 * @return 引擎实例
 */
NERTC_SDK_API nertc_sdk_engine_t nertc_create_engine(const nertc_sdk_config_t *cfg);

/**
 * @brief 销毁引擎实例
 * @param engine 通过nertc_create_engine创建的引擎实例
 */
NERTC_SDK_API void nertc_destory_engine(nertc_sdk_engine_t engine);

/**
 * @brief 初始化引擎实例
 * @note  创建引擎实例之后调用的第一个方法，仅能被初始化一次
 * @param engine 通过nertc_create_engine创建且未被初始化的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_init(nertc_sdk_engine_t engine);

/**
 * @brief 加入房间
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
 * @param channel_name 房间名
 * @param uid 用户id
 * @param token 动态密钥，用于对加入房间用户进行鉴权验证 <br>
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_join(nertc_sdk_engine_t engine,
                            const char* channel_name, 
                            const char * token,
                            uint64_t uid);

/**
 * @brief 离开房间
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_leave(nertc_sdk_engine_t engine);

/**
 * @brief 推送外部音频辅流数据帧。
 * @note
 * - 该方法需要在加入房间后调用。
 * - 数据帧时长需要匹配 on_join 回调中的 nertc_sdk_recommended_config 结构体成员变量 recommended_audio_config
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
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
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
 * @param stream_type 流类型
 * @param audio_config 音频配置
 * @param audio_encoded_frame 音频编码帧
 * @param audio_rms_level 音频数据音量标记，有效值[0,100]，用于后台ASL选路时参考。默认100
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_push_audio_encoded_frame(nertc_sdk_engine_t engine, 
                                                 nertc_sdk_media_stream_e stream_type, 
                                                 nertc_sdk_audio_config audio_config,
                                                 uint8_t audio_rms_level,
                                                 nertc_sdk_audio_encoded_frame_t* audio_encoded_frame);
/**
 * @brief 推送外部音频参考帧。
 * @note
 * - 该方法需要在加入房间后调用。
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
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
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
 * @param config 字幕对应的配置
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */                                                
NERTC_SDK_API int nertc_start_asr_caption(nertc_sdk_engine_t engine, nertc_sdk_asr_caption_config_t* config);

/**
 * @brief 停止字幕
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_stop_asr_caption(nertc_sdk_engine_t engine);

/**
 * @brief 开始AI服务
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
 * @param config AI服务对应的配置
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_start_ai(nertc_sdk_engine_t engine);

/**
 * @brief 停止AI服务
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_stop_ai(nertc_sdk_engine_t engine);

/**
 * @brief 手动打断AI
 * @param engine 通过nertc_create_engine创建且通过nertc_init初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_API int nertc_ai_manual_interrupt(nertc_sdk_engine_t engine);


#ifdef __cplusplus
}
#endif
#endif