#ifndef __NERTC_SDK_DEFINE_H__
#define __NERTC_SDK_DEFINE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** token 最大长度 */
#define kNERtcMaxTokenLength 256

typedef enum {
  NERTC_SDK_LOG_NONE = 0,
  NERTC_SDK_LOG_ERROR = 1,
  NERTC_SDK_LOG_WARNING = 2,
  NERTC_SDK_LOG_INFO = 3
} nertc_sdk_log_level_e;

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
  NERTC_SDK_AUDIO_ENCODE_OPUS = 111,
} nertc_sdk_audio_encode_payload_e;

/** 实时字幕状态 */
typedef enum {
  NERTC_SDK_ASR_CAPTION_START_FAILED = 0,
  NERTC_SDK_ASR_CAPTION_STOP_FAILED = 1,
  NERTC_SDK_ASR_CAPTION_STATE_STARTED = 2,
  NERTC_SDK_ASR_CAPTION_STATE_STOPPED = 3
} nertc_sdk_asr_caption_state_e;

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

typedef struct nertc_sdk_audio_config {
  /** 音频采样率 */
  int sample_rate;
  /** 音频帧时长，单位为毫秒 */
  int frame_duration;
  /** 音频声道数 */
  int channels;
  /** 每个声道的采样点数 */
  int samples_per_channel;
} nertc_sdk_audio_config_t;

typedef struct nertc_sdk_recommended_config {
  /** 走服务端 AEC 时需要参考的音频采集配置 */
  nertc_sdk_audio_config recommended_audio_config;
} nertc_sdk_recommended_config_t;

typedef struct nertc_sdk_audio_frame {
  /** 音频PCM格式 */
  nertc_sdk_audio_pcm_type_e type;
  /** 音频配置信息 */
  nertc_sdk_audio_config config;
  /** 音频帧数据 */
  void* data;
  /** 音频帧数据的长度 */
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
  int encoded_timestamp;
} nertc_sdk_audio_encoded_frame_t;

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

#ifdef __cplusplus
}
#endif

#endif // __NERTC_SDK_DEFINE_H__