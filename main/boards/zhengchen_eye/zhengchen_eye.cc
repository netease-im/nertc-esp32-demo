#include "dual_network_board.h"
#include "display/lcd_display.h"
#include "audio_codecs/box_audio_codec.h"
#include "application.h"
#include "button.h"
#include "led/single_led.h"
#include "config.h"
#include "assets/lang_config.h"
#include "font_awesome_symbols.h"
#include "iot/thing_manager.h"

#include <esp_lcd_panel_vendor.h>
#include <wifi_station.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/spi_common.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>

#include "power_save_timer.h"
#include "../zhengchen-1.54tft-wifi/power_manager.h"
#include "driver/touch_pad.h"

#if CONFIG_USE_LCD_240X240
#include <esp_lcd_gc9a01.h>
static const gc9a01_lcd_init_cmd_t gc9107_lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {0xfe, (uint8_t[]){0x00}, 0, 0},
    {0xef, (uint8_t[]){0x00}, 0, 0},
    {0xb0, (uint8_t[]){0xc0}, 1, 0},
    {0xb1, (uint8_t[]){0x80}, 1, 0},
    {0xb2, (uint8_t[]){0x27}, 1, 0},
    {0xb3, (uint8_t[]){0x13}, 1, 0},
    {0xb6, (uint8_t[]){0x19}, 1, 0},
    {0xb7, (uint8_t[]){0x05}, 1, 0},
    {0xac, (uint8_t[]){0xc8}, 1, 0},
    {0xab, (uint8_t[]){0x0f}, 1, 0},
    {0x3a, (uint8_t[]){0x05}, 1, 0},
    {0xb4, (uint8_t[]){0x04}, 1, 0},
    {0xa8, (uint8_t[]){0x08}, 1, 0},
    {0xb8, (uint8_t[]){0x08}, 1, 0},
    {0xea, (uint8_t[]){0x02}, 1, 0},
    {0xe8, (uint8_t[]){0x2A}, 1, 0},
    {0xe9, (uint8_t[]){0x47}, 1, 0},
    {0xe7, (uint8_t[]){0x5f}, 1, 0},
    {0xc6, (uint8_t[]){0x21}, 1, 0},
    {0xc7, (uint8_t[]){0x15}, 1, 0},
    {0xf0,
    (uint8_t[]){0x1D, 0x38, 0x09, 0x4D, 0x92, 0x2F, 0x35, 0x52, 0x1E, 0x0C,
                0x04, 0x12, 0x14, 0x1f},
    14, 0},
    {0xf1,
    (uint8_t[]){0x16, 0x40, 0x1C, 0x54, 0xA9, 0x2D, 0x2E, 0x56, 0x10, 0x0D,
                0x0C, 0x1A, 0x14, 0x1E},
    14, 0},
    {0xf4, (uint8_t[]){0x00, 0x00, 0xFF}, 3, 0},
    {0xba, (uint8_t[]){0xFF, 0xFF}, 2, 0},
};

#else
#include "esp_lcd_gc9d01n.h"
#endif

#include "mpu6050.h"
#include "driver/i2c.h"
#include <math.h>
#include "freertos/timers.h"
#include "driver/gpio.h"

#define TAG "zhengchen_eye"

LV_FONT_DECLARE(font_puhui_16_4);
LV_FONT_DECLARE(font_awesome_16_4);


class zhengchen_eye : public DualNetworkBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    Button boot_button_;
    LcdDisplay* display_;
    PowerSaveTimer* power_save_timer_;
    PowerManager* power_manager_;
    esp_lcd_panel_io_handle_t panel_io = nullptr;
    esp_lcd_panel_handle_t panel = nullptr;
    uint32_t touch_value = 0;
    uint32_t touch_value1 = 0;
    // 将旧的 mpu6050_handle_t 成员替换为 driver_ng 句柄
    i2c_master_bus_handle_t i2c_mpu_bus_ = nullptr;
    i2c_master_dev_handle_t mpu_dev_ = nullptr;
    bool mpu_init_ = false;
    // 新增：电机控制用定时器
    TimerHandle_t motor_timer_ = nullptr;

    bool MpuWriteReg(uint8_t reg, uint8_t value) {
        if (!mpu_dev_) return false;
        uint8_t buf[2] = {reg, value};
        esp_err_t err = i2c_master_transmit(mpu_dev_, buf, sizeof(buf), 1000);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "MPU6050 write 0x%02X failed: %s", reg, esp_err_to_name(err));
            return false;
        }
        return true;
    }

    bool MpuReadRegs(uint8_t reg, uint8_t* data, size_t len) {
        if (!mpu_dev_) return false;
        esp_err_t err = i2c_master_transmit_receive(mpu_dev_, &reg, 1, data, len, 1000);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "MPU6050 read 0x%02X failed: %s", reg, esp_err_to_name(err));
            return false;
        }
        return true;
    }
    
    // 新增：电机定时器回调（时间到 -> 电机关闭）
    static void MotorTimerCb(TimerHandle_t xTimer) {
        auto* self = static_cast<zhengchen_eye*>(pvTimerGetTimerID(xTimer));
        if (self) {
            gpio_set_level(MOTOR_GPIO, 0); // 低电平 -> 关闭电机
        }
    }

    // 新增：初始化电机 GPIO 与定时器
    void InitializeMotorGpio() {
        gpio_reset_pin(MOTOR_GPIO);
        gpio_set_direction(MOTOR_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(MOTOR_GPIO, 0); // 默认低电平：电机关闭

        motor_timer_ = xTimerCreate("motor_timer",
                                    pdMS_TO_TICKS(MOTOR_ON_DURATION_MS),
                                    pdFALSE,
                                    this,
                                    MotorTimerCb);
        if (!motor_timer_) {
            ESP_LOGE(TAG, "Create motor_timer failed");
        }
    }

    // 新增：触发电机转动 N 毫秒（拉低后启动一次性定时器）
    void MotorStartKick() {
        if (!motor_timer_) return;
        gpio_set_level(MOTOR_GPIO, 1); // 高电平 -> 电机转动
        // 重启一次性定时器（到期后回调会把 GPIO 拉回高电平）
        xTimerStop(motor_timer_, 0);
        xTimerChangePeriod(motor_timer_, pdMS_TO_TICKS(MOTOR_ON_DURATION_MS), 0);
        xTimerStart(motor_timer_, 0);
    }

    void InitializePowerManager() {
        power_manager_ = new PowerManager(GPIO_NUM_7);
        power_manager_->OnChargingStatusChanged([this](bool is_charging) {
            if (is_charging) {
                power_save_timer_->SetEnabled(false);
                ESP_LOGW(TAG, "Charging, disable power save timer");
            } else {
                power_save_timer_->SetEnabled(true);
                ESP_LOGW(TAG, "Not charging, enable power save timer");
            }
        });
    }

    void SleepMode() {
        ESP_LOGI(TAG, "Enabling sleep mode");
        auto display = GetDisplay();
        display->SetChatMessage("system", "");
        GetBacklight()->SetBrightness(0);
        display->SetEmotion("mute");
    }

    void WakeUp() {
        ESP_LOGI(TAG, "Wake up");
        auto display = GetDisplay();
        display->SetChatMessage("system", "");
        display->SetEmotion("neutral");
        GetBacklight()->RestoreBrightness();
    }

    void InitializePowerSaveTimer() {
        power_save_timer_ = new PowerSaveTimer(240, 60, -1);
        power_save_timer_->OnEnterSleepMode([this]() {
            SleepMode();
        });
        power_save_timer_->OnExitSleepMode([this]() {
            WakeUp();
        });
         
        power_save_timer_->SetEnabled(true);
    }

    void InitializeMpu6050() {
        // 使用 driver_ng（i2c_master）在 I2C1 上初始化 MPU6050
        i2c_master_bus_config_t buscfg = {
            .i2c_port = MPU6050_MASTER_NUM,
            .sda_io_num = MPU6050_SDA_GPIO,
            .scl_io_num = MPU6050_SCL_GPIO,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        esp_err_t err = i2c_new_master_bus(&buscfg, &i2c_mpu_bus_);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "MPU6050 i2c_new_master_bus failed: %s", esp_err_to_name(err));
            return;
        }

        i2c_device_config_t devcfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = MPU6050_I2C_ADDR,
            .scl_speed_hz = MPU6050_MASTER_FREQ_HZ,
        };
        err = i2c_master_bus_add_device(i2c_mpu_bus_, &devcfg, &mpu_dev_);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "MPU6050 add device failed: %s", esp_err_to_name(err));
            return;
        }

        // 唤醒 + 量程配置（与原先 ACCE_FS_4G、GYRO_FS_500DPS 保持一致）
        if (!MpuWriteReg(0x6B, 0x00)) { // PWR_MGMT_1 = 0x00（唤醒）
            ESP_LOGE(TAG, "MPU6050 wakeup failed");
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        MpuWriteReg(0x1C, 0x08); // ACCEL_CONFIG = 0x08 -> ±4g
        MpuWriteReg(0x1B, 0x08); // GYRO_CONFIG  = 0x08 -> ±500 dps

        ESP_LOGI(TAG, "MPU6050 initialized on I2C port %d (SDA=%d, SCL=%d)",
                 MPU6050_MASTER_NUM, MPU6050_SDA_GPIO, MPU6050_SCL_GPIO);
        mpu_init_ = true;
    }

    void touch_init() {
        touch_pad_init();
        touch_pad_config(TOUCH_PAD_NUM4); // 配置 GPIO4 为触摸引脚
        touch_pad_config(TOUCH_PAD_NUM5); // 配置 GPIO5 为触摸引脚
        touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER); // 设置 FSM 模式为定时器模式
        touch_pad_fsm_start();
        vTaskDelay(40 / portTICK_PERIOD_MS);
    }

    static void touch_read_task(void* arg) {
        zhengchen_eye* self = static_cast<zhengchen_eye*>(arg);
        auto& app = Application::GetInstance();
        while (1) {
            touch_pad_read_raw_data(TOUCH_PAD_NUM4, &self->touch_value);
            touch_pad_read_raw_data(TOUCH_PAD_NUM5, &self->touch_value1);
            // if (self->touch_value > 31000) {
            //     if (app.GetDeviceState() == kDeviceStateIdle) {
            //         app.WakeWordInvoke("(正在抚摸你的头，请提供相关的情绪价值，回答)");
            //     }
            // }

            // if (self->touch_value1 > 31000) {
            //     if (app.GetDeviceState() == kDeviceStateIdle) {
            //         app.WakeWordInvoke("(正在抚摸你的身体，请提供相关的情绪价值，回答)");
            //     }
            // }
            app.TouchActive(self->touch_value, self->touch_value1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }

    static void mpu6050_read_task(void* arg) {
        zhengchen_eye* self = static_cast<zhengchen_eye*>(arg);
        auto& app = Application::GetInstance();

        const float g_threshold_high = MPU6050_SHAKE_ACCEL_THRESHOLD;   // 瞬时加速度模长超过 ~2.2g 视为剧烈摇晃
        const float delta_threshold  = MPU6050_SHAKE_THRESHOLD;   // 相邻两次采样模长差超过 ~0.7g 视为明显抖动
        const TickType_t cooldown    = pdMS_TO_TICKS(MPU6050_SHAKE_COOLDOWN_TIME);
        TickType_t last_trigger = 0;

        float last_mag = 1.0f;

        while (1) {
            if (!self->mpu_dev_) {
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }

            uint8_t buf[6];
            if (self->MpuReadRegs(0x3B, buf, sizeof(buf))) { // ACCEL_XOUT_H 起始
                int16_t ax_raw = (int16_t)((buf[0] << 8) | buf[1]);
                int16_t ay_raw = (int16_t)((buf[2] << 8) | buf[3]);
                int16_t az_raw = (int16_t)((buf[4] << 8) | buf[5]);

                // ±4g 对应 8192 LSB/g
                float ax = ax_raw / 8192.0f;
                float ay = ay_raw / 8192.0f;
                float az = az_raw / 8192.0f;

                float mag = sqrtf(ax * ax + ay * ay + az * az);
                float delta = fabsf(mag - last_mag);
                TickType_t now = xTaskGetTickCount();

                bool shake = (mag > g_threshold_high) || (delta > delta_threshold);
                bool cool_ok = (now - last_trigger) > cooldown;

                if (shake && cool_ok) {
                    if (self->power_save_timer_) {
                        self->power_save_timer_->WakeUp();
                    }
                    if (mag > g_threshold_high)
                    {
                        app.Shake(mag, delta, true);
                    }
                    else
                    {
                        app.Shake(mag, delta, false);
                    }
                    
                    
                    
                    last_trigger = now;
                }
                last_mag = mag;
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    void InitializeCodecI2c() {
        // Initialize I2C peripheral
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }

   


    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (GetNetworkType() == NetworkType::WIFI) {
                if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                    // cast to WifiBoard
                    auto& wifi_board = static_cast<WifiBoard&>(GetCurrentBoard());
                    wifi_board.ResetWifiConfiguration();
                }
            }
            app.ToggleChatState();
        });
#if CONFIG_USE_4G_WIFI
        boot_button_.OnMultipleClick([this]() {
            SwitchNetworkType();
        });
#endif
        boot_button_.OnLongPress([this]() {
            if (GetNetworkType() == NetworkType::WIFI) {
                auto& wifi_board = static_cast<WifiBoard&>(GetCurrentBoard());
                wifi_board.ResetWifiConfiguration();
            }
            
        });

#if CONFIG_USE_DEVICE_AEC
        boot_button_.OnDoubleClick([this]() {
            auto& app = Application::GetInstance();
            app.SetAecMode(app.GetAecMode() == kAecOff ? kAecOnDeviceSide : kAecOff);
            
        });
#endif
    }

    // 物联网初始化，添加对 AI 可见设备
    void InitializeIot() {
        auto& thing_manager = iot::ThingManager::GetInstance();
        thing_manager.AddThing(iot::CreateThing("Speaker"));
        thing_manager.AddThing(iot::CreateThing("Screen"));
    }

    void InitializeSpi() {
        spi_bus_config_t buscfg = {};
        buscfg.mosi_io_num = DISPLAY_SDA;
        buscfg.miso_io_num = GPIO_NUM_NC;
        buscfg.sclk_io_num = DISPLAY_SCL;
        buscfg.quadwp_io_num = GPIO_NUM_NC;
        buscfg.quadhd_io_num = GPIO_NUM_NC;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeGc9107Display(){
       
        // 液晶屏控制IO初始化
        ESP_LOGD(TAG, "Install panel IO");
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = -1;
        io_config.dc_gpio_num = DISPLAY_DC;
        io_config.spi_mode = 0;
        io_config.pclk_hz = 40 * 1000 * 1000;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &panel_io));

        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RES;
        panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR;
        panel_config.bits_per_pixel = 16;
#if CONFIG_USE_LCD_240X240
        ESP_LOGI(TAG, "Install LCD driver 240");
        // gc9a01_vendor_config_t gc9107_vendor_config = {
        //     .init_cmds = gc9107_lcd_init_cmds,
        //     .init_cmds_size = sizeof(gc9107_lcd_init_cmds) / sizeof(gc9a01_lcd_init_cmd_t),
        // };
        // panel_config.vendor_config = &gc9107_vendor_config;
        ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(panel_io, &panel_config, &panel));
#else
        ESP_LOGI(TAG, "Install LCD driver 160");
        ESP_ERROR_CHECK(esp_lcd_new_panel_gc9d01n(panel_io, &panel_config, &panel));
#endif
        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

        display_ = new SpiLcdDisplay(panel_io, panel,
            DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY,
            {
                .text_font = &font_puhui_16_4,
                .icon_font = &font_awesome_16_4,
                .emoji_font = DISPLAY_HEIGHT >= 240 ? font_emoji_64_init() : font_emoji_32_init(),
            });
        display_->SetEmotion("wifi");
    }

public:
    zhengchen_eye() : 
#if CONFIG_USE_WIFI
        DualNetworkBoard(ML307_TX_PIN, ML307_RX_PIN, GPIO_NUM_NC, 0),
#else
        DualNetworkBoard(ML307_TX_PIN, ML307_RX_PIN),
#endif
        boot_button_(BOOT_BUTTON_GPIO){
        InitializePowerManager();
        InitializePowerSaveTimer();
        InitializeCodecI2c();
        InitializeButtons();
        InitializeIot();
#if !CONFIG_USE_NOLCD
        InitializeSpi();
        InitializeGc9107Display();
        GetBacklight()->SetBrightness(100);
#endif
        GetAudioCodec()->SetOutputVolume(100);
        touch_init();
        xTaskCreate(touch_read_task, "touch_read_task", 2048, this, 5, NULL);
        InitializeMpu6050();
        if (mpu_init_) {
            xTaskCreate(mpu6050_read_task, "mpu6050_read_task", 4096, this, 5, NULL);
            InitializeMotorGpio();
        }
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
        static BoxAudioCodec audio_codec(
            i2c_bus_, 
            AUDIO_INPUT_SAMPLE_RATE, 
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, 
            AUDIO_I2S_GPIO_BCLK, 
            AUDIO_I2S_GPIO_WS, 
            AUDIO_I2S_GPIO_DOUT, 
            AUDIO_I2S_GPIO_DIN,
            GPIO_NUM_NC, 
            AUDIO_CODEC_ES8311_ADDR, 
            AUDIO_CODEC_ES7210_ADDR, 
            AUDIO_INPUT_REFERENCE);
        return &audio_codec;
    }
    
#if !CONFIG_USE_NOLCD
    virtual Display* GetDisplay() override {
        return display_;
    }
    
    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }
#endif

    virtual bool GetBatteryLevel(int& level, bool& charging, bool& discharging) override {
        static bool last_discharging = false;
        charging = power_manager_->IsCharging();
        discharging = power_manager_->IsDischarging();
        if (discharging != last_discharging) {
            power_save_timer_->SetEnabled(discharging);
            last_discharging = discharging;
        }
        level = power_manager_->GetBatteryLevel();
        return true;
    }
  

    virtual bool GetTemperature(float& esp32temp)  override {
        esp32temp = power_manager_->GetTemperature();
        return true;
    }

    // virtual bool Gethead_value(uint32_t& head_value)  override {
    //     head_value = touch_value;
    //     printf("Touch1 value: %ld\n", touch_value);
    //     return true;
    // }

    // virtual bool Getbody_value(uint32_t& body_value)  override {
    //     body_value = touch_value1;
    //     printf("Touch2 value: %ld\n", touch_value1);
    //     return true;
    // }
    // 新增：根据大模型情绪触发电机
    virtual void OnEmotion(const char* emotion) override {
        if (!emotion) return;
        // 命中关键词则触发一次电机；定时器到期会自动关闭
        if (strcmp(emotion, "angry") == 0 ||
            strcmp(emotion, "anger") == 0 ||
            strcmp(emotion, "excited") == 0 ||
            strcmp(emotion, "aggressive") == 0 ||
            strstr(emotion, "怒") != nullptr ||
            strstr(emotion, "激动") != nullptr ||
            strstr(emotion, "兴奋") != nullptr) {
            MotorStartKick();
        }
    }

    virtual void SetPowerSaveMode(bool enabled) override {
        // power_save_timer_->SetEnabled(enabled);
        if (!enabled) {
            power_save_timer_->WakeUp();
            WakeUp();
        } else {
            SleepMode();
        }
        DualNetworkBoard::SetPowerSaveMode(enabled);
    }
};

DECLARE_BOARD(zhengchen_eye);
