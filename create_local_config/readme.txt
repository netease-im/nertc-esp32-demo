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
	"appkey":"xxxxxxx", //必填项，appkey
}
创建配置bin 和烧入配置bin使用的命令
python $IDF_PATH/components/spiffs/spiffsgen.py 0x1000 local_config config.bin
esptool.py --chip esp32-s3 write_flash 0x576000 config.bin
