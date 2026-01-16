#ifndef __NERTC_SDK_DEPRECATED_H__
#define __NERTC_SDK_DEPRECATED_H__

#include "nertc_sdk_define.h"
#include "nertc_sdk_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nertc_sdk_optional_config {
  nertc_sdk_device_level_e device_performance_level;
  bool enable_server_aec;
  bool enable_ptt_mode;
  nertc_sdk_ext_net_handle_t* ext_net_handle;  // 用户自定义的网络接口
  const char* custom_config;
} nertc_sdk_optional_config_t;

typedef struct nertc_sdk_config {
  const char* app_key;                         /**< 应用的AppKey */
  const char* device_id;                       /**< 设备ID */
  bool force_unsafe_mode;                      /**< 是否强制使用非安全模式,
                                                true： 在调用 nertc_join 接口时允许传递空的token，但有可能会出现串房间的问题；
                                                false：在调用 nertc_join 接口时不允许传递空的token，否则会加入房间失败（除非后台针对app key
                                                开启了非安全模式） */
  nertc_sdk_event_handle_t event_handler;      /**< 事件回调 */
  nertc_sdk_user_data_t user_data;             /**< 用户数据 */
  nertc_sdk_optional_config_t optional_config; /**< 可选功能配置 */
  nertc_sdk_licence_config_t licence_cfg;      /**< licence 配置 */
  nertc_sdk_audio_config_t audio_config;       /**< 走设备本地 AEC 时的音频配置 */
  nertc_sdk_log_config_t log_cfg;              /**< 日志配置 */
} nertc_sdk_config_t;

#define NERTC_SDK_DEPRECATED_API __attribute__((visibility("default")))

/**
 * @brief 创建引擎实例,该方法是整个SDK调用的第一个方法（已废弃，请使用 nertc_create_engine_with_config 接口）
 * @param cfg 引擎配置
 * @return 引擎实例
 */
NERTC_SDK_DEPRECATED_API nertc_sdk_engine_t nertc_create_engine(const nertc_sdk_config_t* cfg);

/**
 * @brief 初始化引擎实例 （已废弃，请使用 nertc_init_engine 接口）
 * @note  创建引擎实例之后调用的第一个方法，仅能被初始化一次
 * @param engine 通过 nertc_create_engine 创建且未被初始化的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_DEPRECATED_API int nertc_init(nertc_sdk_engine_t engine);

/**
 * @brief 开始AI服务（已废弃，请使用 nertc_start_ai_with_config 接口）
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_DEPRECATED_API int nertc_start_ai(nertc_sdk_engine_t engine);

/**
 * @brief 挂断电话 （已废弃，请使用 nertc_ai_manual_interrupt 接口）
 * @param engine 通过 nertc_create_engine 创建且通过 nertc_init 初始化之后的引擎实例
 * @return 方法调用结果：<br>
 *         -   0：成功 <br>
 *         - 非0：失败 <br>
 */
NERTC_SDK_DEPRECATED_API int nertc_ai_hang_up(nertc_sdk_engine_t engine);

#ifdef __cplusplus
}
#endif

#endif  // __NERTC_SDK_DEPRECATED_H__
