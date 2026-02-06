#ifndef __NERTC_SDK_DEFINE_H__
#define __NERTC_SDK_DEFINE_H__

#include <stdint.h>

#include "nertc_sdk_ext_net.h"

#ifdef __cplusplus
extern "C" {
#endif

/** token 最大长度 */
#define kNERtcMaxTokenLength 256
#define kNERtcImageLlmMaxFragmentLen 1024

typedef enum {
  NERTC_SDK_DEVICE_LEVEL_NORMAL = 0,
  NERTC_SDK_DEVICE_LEVEL_LOW = 1,
  NERTC_SDK_DEVICE_LEVEL_HIGH = 2
} nertc_sdk_device_level_e;

typedef enum {
  NERTC_SDK_LOG_NONE = 0,
  NERTC_SDK_LOG_ERROR = 1,
  NERTC_SDK_LOG_WARNING = 2,
  NERTC_SDK_LOG_INFO = 3
} nertc_sdk_log_level_e;

typedef enum {
  NERTC_SDK_ENGINE_MODE_AI = 0,
  NERTC_SDK_ENGINE_MODE_PTT = 1,
} nertc_sdk_engine_mode_e;

typedef enum {
  NERTC_SDK_CHANNEL_STATE_IDLE = 0,
  NERTC_SDK_CHANNEL_STATE_JOINING = 1,
  NERTC_SDK_CHANNEL_STATE_JOINED = 2,
  NERTC_SDK_CHANNEL_STATE_LEAVING = 3,
  NERTC_SDK_CHANNEL_STATE_LEAVE = 4,
  NERTC_SDK_CHANNEL_STATE_REJOINING = 5,
  NERTC_SDK_CHANNEL_STATE_JOIN_FAILED = 6
} nertc_sdk_channel_state_e;

typedef enum {
  NERTC_SDK_USER_RTC = 0,
  NERTC_SDK_USER_AI = 1,
  NERTC_SDK_USER_SIP = 2,
} nertc_sdk_user_type_e;

typedef enum {
  NERTC_SDK_MEDIA_UNKNOWN = -1,
  NERTC_SDK_MEDIA_MAIN_AUDIO = 0,
  NERTC_SDK_MEDIA_SUB_AUDIO = 1,
  NERTC_SDK_MEDIA_MAIN_VIDEO = 2,
  NERTC_SDK_MEDIA_SUB_VIDEO = 3,
} nertc_sdk_media_stream_e;

/** PCM 音频格式 */
typedef enum {
  NERTC_SDK_AUDIO_PCM_16 = 0,
} nertc_sdk_audio_pcm_type_e;

typedef enum {
  NERTC_SDK_AUDIO_CODEC_TYPE_PCM = 0,
  NERTC_SDK_AUDIO_CODEC_TYPE_OPUS = 1,
  NERTC_SDK_AUDIO_CODEC_TYPE_G711 = 2,
} nertc_sdk_audio_codec_type_e;

typedef enum {
  NERTC_SDK_AUDIO_ENCODE_OPUS = 111,
  NERTC_SDK_AUDIO_ENCODE_G711 = 8,
} nertc_sdk_audio_encode_payload_e;

/** 实时字幕状态 */
typedef enum {
  NERTC_SDK_ASR_CAPTION_START_FAILED = 0,
  NERTC_SDK_ASR_CAPTION_STOP_FAILED = 1,
  NERTC_SDK_ASR_CAPTION_STATE_STARTED = 2,
  NERTC_SDK_ASR_CAPTION_STATE_STOPPED = 3
} nertc_sdk_asr_caption_state_e;

typedef enum {
  NERTC_SDK_LLM_IMAGE_TYPE_PIC = 0,
  NERTC_SDK_LLM_IMAGE_TYPE_NETWORK_URL = 1,
} nertc_sdk_llm_image_type_e;

typedef enum {
  NERTC_SDK_LLM_IMAGE_EXPLAIN_TYPE_AI = 0,
  NERTC_SDK_LLM_IMAGE_EXPLAIN_TYPE_TAG = 1,
} nertc_sdk_llm_image_explain_type_e;

typedef struct nertc_sdk_licence_config {
  const char* license;  // licence
} nertc_sdk_licence_config_t;

typedef struct nertc_sdk_audio_config {
  /** 音频采样率 */
  int sample_rate;
  /** 音频帧时长，单位为毫秒 */
  int frame_duration;
  /** 音频声道数 */
  int channels;
  /** 每个声道的采样点数 */
  int samples_per_channel;
  /** 接收音频采样率 */
  int out_sample_rate;
  /** 音频编码类型 */
  nertc_sdk_audio_codec_type_e codec_type;
} nertc_sdk_audio_config_t;

typedef struct nertc_sdk_log_config {
  nertc_sdk_log_level_e log_level;
} nertc_sdk_log_config_t;

typedef struct nertc_sdk_optional_configuration {
  nertc_sdk_device_level_e device_performance_level;
  bool enable_server_aec;
  bool prefer_use_psram; /**< 是否优先使用PSRAM */
  const char* custom_config;
} nertc_sdk_optional_configuration_t;

typedef struct nertc_sdk_configuration {
  const char* app_key;    /**< 应用的AppKey */
  const char* device_id;  /**< 设备ID */
  bool force_unsafe_mode; /**< 是否强制使用非安全模式. true: 在调用 nertc_join
                             接口时允许传递空的token，但有可能会出现串房间的问题; false: 在调用 nertc_join
                             接口时不允许传递空的token，否则会加入房间失败（除非后台针对app key 开启了非安全模式） */

  nertc_sdk_licence_config_t licence_cfg;             /**< licence 配置 */
  nertc_sdk_audio_config_t audio_config;              /**< 走设备本地 AEC 时的音频配置 */
  nertc_sdk_optional_configuration_t optional_config; /**< 可选功能配置 */
  nertc_sdk_log_config_t log_cfg;                     /**< 日志配置 */
} nertc_sdk_configuration_t;

typedef struct nertc_sdk_engine_feature_config {
  bool enable_mcp_server; /**< 是否开启 mcp server 功能 */
} nertc_sdk_engine_feature_config_t;

typedef struct nertc_sdk_user_info {
  /**
   * @if Chinese
   * 用户ID
   * @endif
   */
  uint64_t uid;
  /**
   * @if Chinese
   * 用户名字，保留
   * @endif
   */
  const char* name;
  /**
   * @if Chinese
   * 用户类型
   * @endif
   */
  nertc_sdk_user_type_e type;
} nertc_sdk_user_info_t;

typedef uint8_t nertc_sdk_audio_data_t;

typedef struct nertc_sdk_recommended_config {
  /** 走服务端 AEC 时需要参考的音频采集配置 */
  nertc_sdk_audio_config_t recommended_audio_config;
} nertc_sdk_recommended_config_t;

typedef struct nertc_sdk_audio_frame {
  /** 音频PCM格式 */
  nertc_sdk_audio_pcm_type_e type;
  /** 音频配置信息 */
  nertc_sdk_audio_config_t config;
  /** 音频帧数据 */
  void* data;
  /** 音频帧数据int16的长度 */
  int length;
} nertc_sdk_audio_frame_t;

typedef struct nertc_sdk_audio_encoded_frame {
  /** 编码音频帧的数据 */
  nertc_sdk_audio_data_t* data;
  /** 编码音频帧的数据长度 */
  int length;
  /** 编码音频帧的payload类型，详细信息请参考 nertc_sdk_audio_encode_payload_e */
  nertc_sdk_audio_encode_payload_e payload_type;
  /** 编码音频帧的时间戳，单位为毫秒 */
  int64_t timestamp_ms;
  /** 编码时间，单位为样本数，如0、960、1920...递增 */
  uint32_t encoded_timestamp;
} nertc_sdk_audio_encoded_frame_t;

typedef struct nertc_sdk_start_ai_config {
  const char* start_topic;
  size_t start_topic_len;
} nertc_sdk_start_ai_config_t;


typedef struct nertc_sdk_asr_caption_config {
  /** 字幕的源语言，默认为AUTO */
  char src_language[kNERtcMaxTokenLength];
  /** 字幕的目标语言。默认为空，不翻译 */
  char dst_language[kNERtcMaxTokenLength];
} nertc_sdk_asr_caption_config_t;

typedef struct nertc_sdk_asr_caption_result {
  /** 来源的用户 ID */
  uint64_t user_id;
  /** 是否为本地用户 */
  bool is_local_user;
  /** 实时字幕时间戳 */
  uint64_t timestamp;
  /** 实时字幕内容 */
  const char* content;
  /** 实时字幕语言 */
  const char* language;
  /** 是否包含翻译 */
  bool have_translation;
  /** 翻译后的字幕内容 */
  const char* translated_text;
  /** 翻译后的字幕语言 */
  const char* translation_language;
  /** 是否为最终结果 */
  bool is_final;
} nertc_sdk_asr_caption_result_t;

typedef struct nertc_sdk_ai_data_result {
  /** 类型 */
  const char* type;
  /** 类型长度 */
  int type_len;
  /** AI数据 */
  const char* data;
  /** AI数据长度 */
  int data_len;
} nertc_sdk_ai_data_result_t;

typedef struct nertc_sdk_ai_llm_request {
  /** 文本 */
  const char* text;
  /** 图片base64字符串 或 网络图片url */
  const char* img_url;
  /** 图片base64字符串长度 或者 网络图片url长度 */
  int img_len;
  /** 图片压缩方式, 0: 不压缩, 1: gzip*/
  int img_compress_type;
  /** 打断模式 1:直接打断, 2:等待当前交互结束后, 3:直接丢弃*/
  int interrupt_mode;
  /** 图片类型 */
  nertc_sdk_llm_image_type_e img_type;
  /** 图片解释类型, 0:AI回答，1:标签识图，通过on_ai_data返回photoTag类型JSON数据，此时 interrupt_mode 和 text 暂时无效 */
  nertc_sdk_llm_image_explain_type_e img_explain_type;
} nertc_sdk_ai_llm_request_t;

typedef struct nertc_sdk_mcp_tool_result {
  const char* payload;  // 工具调用结果, 遵循 JSON-RPC 2.0 规范
  int payload_len;      // 工具调用结果的长度
} nertc_sdk_mcp_tool_result_t;


#ifdef __cplusplus
}
#endif

#endif  // __NERTC_SDK_DEFINE_H__
