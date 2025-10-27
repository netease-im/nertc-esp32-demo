example:
{
	"audio_config": { // 音频编解码相关的配置
		"frame_size": 20
	},
	"license_config": { // 证书配置
		"license": ""
	},
	"wake_words": [ //唤醒配置
		"ni hao chong chong",
		"ni hao dan zai"
	],
	"appkey":"eca23f68c66d4acfceee77c200200359", //必填项，appkey
	"custom_config": {
		"test_mode": true, //是否测试环境
        "enable_format_log_time" : false, 使用打印日期时间
        "mps_debug" : "xxxxxx", 基本不用填
	}
}
创建配置bin 和烧入配置bin使用的命令
python $IDF_PATH/components/spiffs/spiffsgen.py 0x1000 local_config config.bin
esptool.py --chip esp32-s3 write_flash 0x576000 config.bin
