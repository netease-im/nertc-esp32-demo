// Auto-generated language config
#pragma once

#include <string_view>

#ifndef zh_cn
    #define zh_cn  // 預設語言
#endif

namespace Lang {
    // 语言元数据
    constexpr const char* CODE = "zh-CN";

    // 字符串资源
    namespace Strings {
        constexpr const char* ACCESS_VIA_BROWSER = "，浏览器访问 ";
        constexpr const char* ACTIVATION = "激活设备";
        constexpr const char* BATTERY_CHARGING = "正在充电";
        constexpr const char* BATTERY_FULL = "电量已满";
        constexpr const char* BATTERY_LOW = "电量不足";
        constexpr const char* BATTERY_NEED_CHARGE = "电量低，请充电";
        constexpr const char* CHECKING_NEW_VERSION = "检查新版本...";
        constexpr const char* CHECK_NEW_VERSION_FAILED = "检查新版本失败，将在 %d 秒后重试：%s";
        constexpr const char* CONNECTED_TO = "已连接 ";
        constexpr const char* CONNECTING = "连接中...";
        constexpr const char* CONNECT_TO = "连接 ";
        constexpr const char* CONNECT_TO_HOTSPOT = "手机连接热点 ";
        constexpr const char* DETECTING_MODULE = "检测模组...";
        constexpr const char* ENTERING_WIFI_CONFIG_MODE = "进入配网模式...";
        constexpr const char* ERROR = "错误";
        constexpr const char* INFO = "信息";
        constexpr const char* INITIALIZING = "正在初始化...";
        constexpr const char* LISTENING = "聆听中...";
        constexpr const char* LOADING_PROTOCOL = "登录服务器...";
        constexpr const char* MAX_VOLUME = "最大音量";
        constexpr const char* MUTED = "已静音";
        constexpr const char* NEW_VERSION = "新版本 ";
        constexpr const char* OTA_UPGRADE = "OTA 升级";
        constexpr const char* PIN_ERROR = "请插入 SIM 卡";
        constexpr const char* REGISTERING_NETWORK = "等待网络...";
        constexpr const char* REG_ERROR = "无法接入网络，请检查流量卡状态";
        constexpr const char* RTC_MODE_OFF = "AEC 关闭";
        constexpr const char* RTC_MODE_ON = "AEC 开启";
        constexpr const char* SCANNING_WIFI = "扫描 Wi-Fi...";
        constexpr const char* SERVER_ERROR = "发送失败，请检查网络";
        constexpr const char* SERVER_NOT_CONNECTED = "无法连接服务，请稍后再试";
        constexpr const char* SERVER_NOT_FOUND = "正在寻找可用服务";
        constexpr const char* SERVER_TIMEOUT = "等待响应超时";
        constexpr const char* SPEAKING = "说话中...";
        constexpr const char* STANDBY = "待命";
        constexpr const char* SWITCH_TO_4G_NETWORK = "切换到 4G...";
        constexpr const char* SWITCH_TO_WIFI_NETWORK = "切换到 Wi-Fi...";
        constexpr const char* UPGRADE_FAILED = "升级失败";
        constexpr const char* UPGRADING = "正在升级系统...";
        constexpr const char* VERSION = "版本 ";
        constexpr const char* VOLUME = "音量 ";
        constexpr const char* WARNING = "警告";
        constexpr const char* WIFI_CONFIG_MODE = "配网模式";
    }

    // 音效资源
    namespace Sounds {

        extern const char p3_0_start[] asm("_binary_0_p3_start");
        extern const char p3_0_end[] asm("_binary_0_p3_end");
        static const std::string_view P3_0 {
        static_cast<const char*>(p3_0_start),
        static_cast<size_t>(p3_0_end - p3_0_start)
        };

        extern const char p3_1_start[] asm("_binary_1_p3_start");
        extern const char p3_1_end[] asm("_binary_1_p3_end");
        static const std::string_view P3_1 {
        static_cast<const char*>(p3_1_start),
        static_cast<size_t>(p3_1_end - p3_1_start)
        };

        extern const char p3_2_start[] asm("_binary_2_p3_start");
        extern const char p3_2_end[] asm("_binary_2_p3_end");
        static const std::string_view P3_2 {
        static_cast<const char*>(p3_2_start),
        static_cast<size_t>(p3_2_end - p3_2_start)
        };

        extern const char p3_3_start[] asm("_binary_3_p3_start");
        extern const char p3_3_end[] asm("_binary_3_p3_end");
        static const std::string_view P3_3 {
        static_cast<const char*>(p3_3_start),
        static_cast<size_t>(p3_3_end - p3_3_start)
        };

        extern const char p3_4_start[] asm("_binary_4_p3_start");
        extern const char p3_4_end[] asm("_binary_4_p3_end");
        static const std::string_view P3_4 {
        static_cast<const char*>(p3_4_start),
        static_cast<size_t>(p3_4_end - p3_4_start)
        };

        extern const char p3_4g_connected_start[] asm("_binary_4g_connected_p3_start");
        extern const char p3_4g_connected_end[] asm("_binary_4g_connected_p3_end");
        static const std::string_view P3_4G_CONNECTED {
        static_cast<const char*>(p3_4g_connected_start),
        static_cast<size_t>(p3_4g_connected_end - p3_4g_connected_start)
        };

        extern const char p3_5_start[] asm("_binary_5_p3_start");
        extern const char p3_5_end[] asm("_binary_5_p3_end");
        static const std::string_view P3_5 {
        static_cast<const char*>(p3_5_start),
        static_cast<size_t>(p3_5_end - p3_5_start)
        };

        extern const char p3_6_start[] asm("_binary_6_p3_start");
        extern const char p3_6_end[] asm("_binary_6_p3_end");
        static const std::string_view P3_6 {
        static_cast<const char*>(p3_6_start),
        static_cast<size_t>(p3_6_end - p3_6_start)
        };

        extern const char p3_7_start[] asm("_binary_7_p3_start");
        extern const char p3_7_end[] asm("_binary_7_p3_end");
        static const std::string_view P3_7 {
        static_cast<const char*>(p3_7_start),
        static_cast<size_t>(p3_7_end - p3_7_start)
        };

        extern const char p3_8_start[] asm("_binary_8_p3_start");
        extern const char p3_8_end[] asm("_binary_8_p3_end");
        static const std::string_view P3_8 {
        static_cast<const char*>(p3_8_start),
        static_cast<size_t>(p3_8_end - p3_8_start)
        };

        extern const char p3_9_start[] asm("_binary_9_p3_start");
        extern const char p3_9_end[] asm("_binary_9_p3_end");
        static const std::string_view P3_9 {
        static_cast<const char*>(p3_9_start),
        static_cast<size_t>(p3_9_end - p3_9_start)
        };

        extern const char p3_activation_start[] asm("_binary_activation_p3_start");
        extern const char p3_activation_end[] asm("_binary_activation_p3_end");
        static const std::string_view P3_ACTIVATION {
        static_cast<const char*>(p3_activation_start),
        static_cast<size_t>(p3_activation_end - p3_activation_start)
        };

        extern const char p3_agent_start[] asm("_binary_agent_p3_start");
        extern const char p3_agent_end[] asm("_binary_agent_p3_end");
        static const std::string_view P3_AGENT {
        static_cast<const char*>(p3_agent_start),
        static_cast<size_t>(p3_agent_end - p3_agent_start)
        };

        extern const char p3_blufi_start[] asm("_binary_blufi_p3_start");
        extern const char p3_blufi_end[] asm("_binary_blufi_p3_end");
        static const std::string_view P3_BLUFI {
        static_cast<const char*>(p3_blufi_start),
        static_cast<size_t>(p3_blufi_end - p3_blufi_start)
        };

        extern const char p3_chongchong_wozai_start[] asm("_binary_chongchong_wozai_p3_start");
        extern const char p3_chongchong_wozai_end[] asm("_binary_chongchong_wozai_p3_end");
        static const std::string_view P3_CHONGCHONG_WOZAI {
        static_cast<const char*>(p3_chongchong_wozai_start),
        static_cast<size_t>(p3_chongchong_wozai_end - p3_chongchong_wozai_start)
        };

        extern const char p3_chongchong_wozaiting_start[] asm("_binary_chongchong_wozaiting_p3_start");
        extern const char p3_chongchong_wozaiting_end[] asm("_binary_chongchong_wozaiting_p3_end");
        static const std::string_view P3_CHONGCHONG_WOZAITING {
        static_cast<const char*>(p3_chongchong_wozaiting_start),
        static_cast<size_t>(p3_chongchong_wozaiting_end - p3_chongchong_wozaiting_start)
        };

        extern const char p3_chongchong_zaine_start[] asm("_binary_chongchong_zaine_p3_start");
        extern const char p3_chongchong_zaine_end[] asm("_binary_chongchong_zaine_p3_end");
        static const std::string_view P3_CHONGCHONG_ZAINE {
        static_cast<const char*>(p3_chongchong_zaine_start),
        static_cast<size_t>(p3_chongchong_zaine_end - p3_chongchong_zaine_start)
        };

        extern const char p3_connected_start[] asm("_binary_connected_p3_start");
        extern const char p3_connected_end[] asm("_binary_connected_p3_end");
        static const std::string_view P3_CONNECTED {
        static_cast<const char*>(p3_connected_start),
        static_cast<size_t>(p3_connected_end - p3_connected_start)
        };

        extern const char p3_dangzai_wozaiting_start[] asm("_binary_dangzai_wozaiting_p3_start");
        extern const char p3_dangzai_wozaiting_end[] asm("_binary_dangzai_wozaiting_p3_end");
        static const std::string_view P3_DANGZAI_WOZAITING {
        static_cast<const char*>(p3_dangzai_wozaiting_start),
        static_cast<size_t>(p3_dangzai_wozaiting_end - p3_dangzai_wozaiting_start)
        };

        extern const char p3_dangzai_zaine_start[] asm("_binary_dangzai_zaine_p3_start");
        extern const char p3_dangzai_zaine_end[] asm("_binary_dangzai_zaine_p3_end");
        static const std::string_view P3_DANGZAI_ZAINE {
        static_cast<const char*>(p3_dangzai_zaine_start),
        static_cast<size_t>(p3_dangzai_zaine_end - p3_dangzai_zaine_start)
        };

        extern const char p3_danzai_nishuo_start[] asm("_binary_danzai_nishuo_p3_start");
        extern const char p3_danzai_nishuo_end[] asm("_binary_danzai_nishuo_p3_end");
        static const std::string_view P3_DANZAI_NISHUO {
        static_cast<const char*>(p3_danzai_nishuo_start),
        static_cast<size_t>(p3_danzai_nishuo_end - p3_danzai_nishuo_start)
        };

        extern const char p3_err_pin_start[] asm("_binary_err_pin_p3_start");
        extern const char p3_err_pin_end[] asm("_binary_err_pin_p3_end");
        static const std::string_view P3_ERR_PIN {
        static_cast<const char*>(p3_err_pin_start),
        static_cast<size_t>(p3_err_pin_end - p3_err_pin_start)
        };

        extern const char p3_err_reg_start[] asm("_binary_err_reg_p3_start");
        extern const char p3_err_reg_end[] asm("_binary_err_reg_p3_end");
        static const std::string_view P3_ERR_REG {
        static_cast<const char*>(p3_err_reg_start),
        static_cast<size_t>(p3_err_reg_end - p3_err_reg_start)
        };

        extern const char p3_exclamation_start[] asm("_binary_exclamation_p3_start");
        extern const char p3_exclamation_end[] asm("_binary_exclamation_p3_end");
        static const std::string_view P3_EXCLAMATION {
        static_cast<const char*>(p3_exclamation_start),
        static_cast<size_t>(p3_exclamation_end - p3_exclamation_start)
        };

        extern const char p3_low_battery_start[] asm("_binary_low_battery_p3_start");
        extern const char p3_low_battery_end[] asm("_binary_low_battery_p3_end");
        static const std::string_view P3_LOW_BATTERY {
        static_cast<const char*>(p3_low_battery_start),
        static_cast<size_t>(p3_low_battery_end - p3_low_battery_start)
        };

        extern const char p3_popup_start[] asm("_binary_popup_p3_start");
        extern const char p3_popup_end[] asm("_binary_popup_p3_end");
        static const std::string_view P3_POPUP {
        static_cast<const char*>(p3_popup_start),
        static_cast<size_t>(p3_popup_end - p3_popup_start)
        };

        extern const char p3_ring_start[] asm("_binary_ring_p3_start");
        extern const char p3_ring_end[] asm("_binary_ring_p3_end");
        static const std::string_view P3_RING {
        static_cast<const char*>(p3_ring_start),
        static_cast<size_t>(p3_ring_end - p3_ring_start)
        };

        extern const char p3_satisfy_start[] asm("_binary_satisfy_p3_start");
        extern const char p3_satisfy_end[] asm("_binary_satisfy_p3_end");
        static const std::string_view P3_SATISFY {
        static_cast<const char*>(p3_satisfy_start),
        static_cast<size_t>(p3_satisfy_end - p3_satisfy_start)
        };

        extern const char p3_success_start[] asm("_binary_success_p3_start");
        extern const char p3_success_end[] asm("_binary_success_p3_end");
        static const std::string_view P3_SUCCESS {
        static_cast<const char*>(p3_success_start),
        static_cast<size_t>(p3_success_end - p3_success_start)
        };

        extern const char p3_upgrade_start[] asm("_binary_upgrade_p3_start");
        extern const char p3_upgrade_end[] asm("_binary_upgrade_p3_end");
        static const std::string_view P3_UPGRADE {
        static_cast<const char*>(p3_upgrade_start),
        static_cast<size_t>(p3_upgrade_end - p3_upgrade_start)
        };

        extern const char p3_vibration_start[] asm("_binary_vibration_p3_start");
        extern const char p3_vibration_end[] asm("_binary_vibration_p3_end");
        static const std::string_view P3_VIBRATION {
        static_cast<const char*>(p3_vibration_start),
        static_cast<size_t>(p3_vibration_end - p3_vibration_start)
        };

        extern const char p3_welcome_start[] asm("_binary_welcome_p3_start");
        extern const char p3_welcome_end[] asm("_binary_welcome_p3_end");
        static const std::string_view P3_WELCOME {
        static_cast<const char*>(p3_welcome_start),
        static_cast<size_t>(p3_welcome_end - p3_welcome_start)
        };

        extern const char p3_wificonfig_start[] asm("_binary_wificonfig_p3_start");
        extern const char p3_wificonfig_end[] asm("_binary_wificonfig_p3_end");
        static const std::string_view P3_WIFICONFIG {
        static_cast<const char*>(p3_wificonfig_start),
        static_cast<size_t>(p3_wificonfig_end - p3_wificonfig_start)
        };

        extern const char p3_wu_curious_start[] asm("_binary_wu_curious_p3_start");
        extern const char p3_wu_curious_end[] asm("_binary_wu_curious_p3_end");
        static const std::string_view P3_WU_CURIOUS {
        static_cast<const char*>(p3_wu_curious_start),
        static_cast<size_t>(p3_wu_curious_end - p3_wu_curious_start)
        };

        extern const char p3_wuwu_start[] asm("_binary_wuwu_p3_start");
        extern const char p3_wuwu_end[] asm("_binary_wuwu_p3_end");
        static const std::string_view P3_WUWU {
        static_cast<const char*>(p3_wuwu_start),
        static_cast<size_t>(p3_wuwu_end - p3_wuwu_start)
        };

        extern const char p3_ying_start[] asm("_binary_ying_p3_start");
        extern const char p3_ying_end[] asm("_binary_ying_p3_end");
        static const std::string_view P3_YING {
        static_cast<const char*>(p3_ying_start),
        static_cast<size_t>(p3_ying_end - p3_ying_start)
        };
    }
}
