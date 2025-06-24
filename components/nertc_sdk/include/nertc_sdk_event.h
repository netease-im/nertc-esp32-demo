#ifndef __NERTC_SDK_EVENT_H__
#define __NERTC_SDK_EVENT_H__

#include "nertc_sdk_define.h"
#include "nertc_sdk_error.h"



#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void * nertc_sdk_engine_t;
typedef void * nertc_sdk_user_data_t;

typedef struct nertc_sdk_callback_context {
  nertc_sdk_engine_t engine;        /**< 引擎实例 */
  nertc_sdk_user_data_t user_data;  /**< 用户数据 */
} nertc_sdk_callback_context_t;

typedef struct nertc_sdk_event_handler {
/**
  * 发生错误回调。
  * <br>该回调方法表示 SDK 运行时出现了（网络或媒体相关的）错误。通常情况下，SDK上报的错误意味着SDK无法自动恢复，需要 App
  * 干预或提示用户。
  * @param ctx 回调上下文
  * @param code 错误码。详细信息请参考 nertc_error_code
  * @param msg 错误描述
  * @endif
  */
void (*on_error)(const nertc_sdk_callback_context_t* ctx, int code, const char *msg);

/**
  * 房间状态已改变回调。
  * <br>该回调在房间连接状态发生改变的时候触发，并告知用户当前的连接状态和引起状态改变的原因。
  * @param ctx 回调上下文
  * @param state 当前的房间连接状态。
  * @param reason 引起当前连接状态发生改变的原因
  * @endif
  */
void (*on_channel_status_changed)(const nertc_sdk_callback_context_t* ctx, nertc_sdk_channel_state_e status, const char *msg);

/**
 * @brief 加入房间成功回调。
* @param ctx 回调上下文
 * @param cid 房间名
 * @param uid 用户id
 * @param code 错误码
 * @param elapsed 从开始加入房间到加入房间成功的耗时，单位：毫秒
 */
void (*on_join)(const nertc_sdk_callback_context_t* ctx, uint64_t cid, uint64_t uid, nertc_sdk_error_code_e code, uint64_t elapsed, const nertc_sdk_recommended_config* recommended_config);

/**
 * @brief 远端用户加入房间回调 <br>
 * @param engine nertc_sdk_engine_t
 * @param uid 远端用户id
 * @param user_name 远端用户名(保留字段)
 */
void (*on_user_joined)(const nertc_sdk_callback_context_t* ctx, const nertc_sdk_user_info& user);

/**
 * @brief 远端用户离开房间<br>
 * @param ctx 回调上下文
 * @param uid 远端用户id
 * @param reason 用户离开房间的原因
 */
void (*on_user_left)(const nertc_sdk_callback_context_t* ctx, const nertc_sdk_user_info& user, int reason);

/**
  * @if Chinese
  * 远端用户开启音频的回调。
  * @param ctx 回调上下文
  * @param uid 远端用户id
  * @param stream_type 远端用户流类型
  * @endif
  */
  void (*on_user_audio_start)(const nertc_sdk_callback_context_t* ctx, uint64_t uid, nertc_sdk_media_stream_e stream_type);

/**
 * @brief 房间内用户静音
 * @param ctx 回调上下文
 * @param uid 远端用户id
 * @param mute 是否静音
 * @endif
 */
void (*on_user_audio_mute)(const nertc_sdk_callback_context_t* ctx, uint64_t uid , bool mute);

/**
 * 远端用户停止音频的回调
 * @param engine nertc_sdk_engine_t
 * @param uid 远端用户id
 * @param stream_type 远端用户流类型
 * @endif
 */
void (*on_user_audio_stop)(const nertc_sdk_callback_context_t* ctx, uint64_t uid, nertc_sdk_media_stream_e stream_type);

/**
 * 实时字幕状态回调。
 * @param ctx 回调上下文
 * @param state asr caption 的状态
 * @param code 具体的错误码，成功为200
 * @param msg 具体的错误信息
 * @endif
 */
void (*on_asr_caption_state_changed)(const nertc_sdk_callback_context_t* ctx, nertc_sdk_asr_caption_state_e state, int code, const char *msg);

/**
 * 实时字幕信息回调。
 * @param ctx 回调上下文
 * @param results 详细的字幕信息数组
 * @param result_count 字幕信息数组的个数
 * @endif
 */
void (*on_asr_caption_result)(const nertc_sdk_callback_context_t* ctx, nertc_sdk_asr_caption_result_t* results, int result_count);

/**
 * 实时AI信令回调。
 * @param ctx 回调上下文
 * @param data 信令数据
 * @endif
 */
void (*on_ai_data)(const nertc_sdk_callback_context_t* ctx, nertc_sdk_ai_data_result_t* ai_data);

/**
 * 远端单个用户的音频解码前数据回调。
 * @param ctx 回调上下文  
 * @param frame 远端单个用户的解码前音频帧
 * @endif
 */
void (*on_audio_encoded_data)(const nertc_sdk_callback_context_t* ctx, uint64_t uid, nertc_sdk_media_stream_e stream_type, nertc_sdk_audio_encoded_frame_t* encoded_frame, bool is_mute_packet);

} nertc_sdk_event_handle_t;

typedef struct nertc_sdk_config {
  const char *app_key;    /**< 应用的AppKey */
  const char *device_id;  /**< 设备ID */
  nertc_sdk_audio_config audio_config;  /**< 走设备本地 AEC 时的音频配置 */
  nertc_sdk_event_handle_t event_handler;
  nertc_sdk_user_data_t user_data;
  // nertc_sdk_log_config_t log_cfg;
} nertc_sdk_config_t;

#ifdef __cplusplus
}
#endif

#endif // __NERTC_SDK_EVENT_H__