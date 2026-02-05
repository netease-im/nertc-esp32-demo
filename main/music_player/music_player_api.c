#include "FreeRTOSConfig.h"
#include "esp_audio_simple_dec.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_gmf_alc.h"
#include "esp_gmf_element.h"
#include "esp_gmf_pipeline.h"
#include "esp_gmf_pool.h"

#include "esp_audio_simple_player.h"
#include "esp_audio_simple_player_advance.h"
#include "esp_codec_dev.h"
#include "esp_gmf_io.h"
#include "esp_gmf_io_embed_flash.h"
#include <stdlib.h>
#include <string.h>
#include "music_player_api.h"

#define TAG "mp3_player"

static esp_asp_handle_t player_handle = NULL;
static esp_asp_state_t player_state;
static esp_asp_music_info_t music_info;
static mp3_player_event_cb_t g_event_cb = NULL;
static mp3_player_output_cb_t g_output_cb = NULL;
static mp3_player_info_cb_t g_info_cb = NULL;
static SemaphoreHandle_t player_mutex = NULL;
static int g_play_size = 0; // 已经播放的长度(字节)
static double g_duration = 0;

static int mock_event_callback(esp_asp_event_pkt_t *event, void *ctx) {
  if (event->type == ESP_ASP_EVENT_TYPE_MUSIC_INFO) {
    esp_asp_music_info_t info = {0};
    memcpy(&info, event->payload, event->payload_size);
    ESP_LOGW(TAG, "Get info, rate:%d, channels:%d, bits:%d", info.sample_rate,
             info.channels, info.bits);
    if(g_info_cb){
      g_info_cb(info.sample_rate, info.channels, info.bits, ctx);
    }
  } else if (event->type == ESP_ASP_EVENT_TYPE_STATE) {
    esp_asp_state_t st = 0;
    memcpy(&st, event->payload, event->payload_size);
    ESP_LOGW(TAG, "Get State, %d,%s", st,
             esp_audio_simple_player_state_to_str(st));
    if (g_event_cb) {
      music_player_state_t mp_state = MUSIC_PLAYER_STATE_NONE;
      switch (st) {
        case ESP_ASP_STATE_NONE:
          mp_state = MUSIC_PLAYER_STATE_NONE;
          break;
        case ESP_ASP_STATE_RUNNING:
          mp_state = MUSIC_PLAYER_STATE_PLAYING;    
          break;
        case ESP_ASP_STATE_PAUSED:
          mp_state = MUSIC_PLAYER_STATE_PAUSED;
          break;
        case ESP_ASP_STATE_STOPPED:
          mp_state = MUSIC_PLAYER_STATE_STOPPED;
          break;
        case ESP_ASP_STATE_FINISHED:
          mp_state = MUSIC_PLAYER_STATE_FINISHED;
          break;
        case ESP_ASP_STATE_ERROR:
          mp_state = MUSIC_PLAYER_STATE_ERROR;
          break;
        default:
          mp_state = MUSIC_PLAYER_STATE_NONE;
          break;
      }
      g_event_cb(mp_state, ctx);
    }
  }
  return 0;
}

static int output_cb(uint8_t *data, int data_size, void *ctx) {
  g_output_cb(data, data_size, ctx);
  // 累计当前播放的大小
  g_play_size += data_size;
  return 0;
}

void mp3_player_init(mp3_player_output_cb_t output_cb_, mp3_player_info_cb_t info_cb, mp3_player_event_cb_t event_cb, void *output_cb_arg) {
  if (player_handle != NULL) {
    return;
  }
  g_info_cb = info_cb;
  g_output_cb = output_cb_;
  g_event_cb = event_cb;
  esp_asp_cfg_t cfg = {.in.cb = NULL,
                       .in.user_ctx = NULL,
                       .out.cb = (esp_asp_data_func)output_cb,
                       .out.user_ctx = output_cb_arg,
                       .task_prio = 5,
                       .task_core = 1,
                       .task_stack = 1024 * 40,
                       .task_stack_in_ext = 1};
  ESP_GMF_MEM_SHOW(TAG);
  esp_audio_simple_player_new(&cfg, &player_handle);
  esp_audio_simple_player_set_event(player_handle, mock_event_callback, output_cb_arg);
  esp_audio_simple_player_get_state(player_handle, &player_state);
  player_mutex = xSemaphoreCreateBinary();
  xSemaphoreGive(player_mutex);
  ESP_LOGD(TAG, "init");
}

void mp3_player_play(const char *mp3_path, bool loop, void* ctx) {
  if (player_mutex == NULL) {
    return;
  }
  g_play_size = 0;
  mp3_player_stop();
  if (xSemaphoreTake(player_mutex, portMAX_DELAY) == pdTRUE) {
    // 先更新总时长
    //if (strncmp(mp3_path, "file:/", 6) == 0) {
    //  g_duration = mp3_file_calculate_duration(mp3_path + 6);
    //}
    // 然后再播放
    esp_gmf_err_t res = esp_audio_simple_player_run(player_handle, mp3_path, &music_info);
    xSemaphoreGive(player_mutex);
    if(res != ESP_GMF_ERR_OK){
      g_event_cb(MUSIC_PLAYER_STATE_ERROR, ctx);
    }
  }
  ESP_LOGD(TAG, "play:%s", mp3_path);
}

void mp3_player_pause() {
  if (player_mutex == NULL) {
    return;
  }
  if (!mp3_player_is_paused()) {
    if (xSemaphoreTake(player_mutex, portMAX_DELAY) == pdTRUE) {
      esp_audio_simple_player_pause(player_handle);
      xSemaphoreGive(player_mutex);
    }
  }
  ESP_LOGD(TAG, "pause");
}

void mp3_player_stop() {
  if (player_mutex == NULL) {
    return;
  }
  g_play_size = 0;
  if (mp3_player_is_playing() || mp3_player_is_paused() ||
      !mp3_player_is_stopped()) {
    if (xSemaphoreTake(player_mutex, portMAX_DELAY) == pdTRUE) {
      esp_audio_simple_player_stop(player_handle);
      xSemaphoreGive(player_mutex);
    }
  }
  ESP_LOGD(TAG, "stop");
}

void mp3_player_resume() {
  if (player_mutex == NULL) {
    return;
  }
  if (mp3_player_is_paused()) {
    if (xSemaphoreTake(player_mutex, portMAX_DELAY) == pdTRUE) {
      esp_audio_simple_player_resume(player_handle);
      xSemaphoreGive(player_mutex);
    }
  }
  ESP_LOGD(TAG, "resume");
}

bool mp3_player_is_playing() {
  if (player_mutex == NULL) {
    return false;
  }
  if (xSemaphoreTake(player_mutex, portMAX_DELAY) == pdTRUE) {
    esp_audio_simple_player_get_state(player_handle, &player_state);
    ESP_LOGD(TAG, "is_playing:%d", player_state == ESP_ASP_STATE_RUNNING);
    xSemaphoreGive(player_mutex);
  }
  return player_state == ESP_ASP_STATE_RUNNING;
}

bool mp3_player_is_paused() {
  if (player_mutex == NULL) {
    return false;
  }
  if (xSemaphoreTake(player_mutex, portMAX_DELAY) == pdTRUE) {
    esp_audio_simple_player_get_state(player_handle, &player_state);
    xSemaphoreGive(player_mutex);
  }
  return player_state == ESP_ASP_STATE_PAUSED;
}

bool mp3_player_is_stopped() {
  if (player_mutex == NULL) {
    return false;
  }
  if (xSemaphoreTake(player_mutex, portMAX_DELAY) == pdTRUE) {
    esp_audio_simple_player_get_state(player_handle, &player_state);
    xSemaphoreGive(player_mutex);
  }
  return player_state == ESP_ASP_STATE_STOPPED;
}

bool mp3_player_is_finnished() {
  if (player_mutex == NULL) {
    return false;
  }
  if (xSemaphoreTake(player_mutex, portMAX_DELAY) == pdTRUE) {
    esp_audio_simple_player_get_state(player_handle, &player_state);
    xSemaphoreGive(player_mutex);
  }

  if(player_state == ESP_ASP_STATE_FINISHED){
    ESP_LOGE(TAG, "---------------------finnished");
  }
  return player_state == ESP_ASP_STATE_FINISHED;
}

unsigned int mp3_player_get_duration() { return g_duration; }

unsigned int mp3_player_get_position() {
  int sample_rate = CONFIG_AUDIO_SIMPLE_PLAYER_RESAMPLE_DEST_RATE;
#define CONFIG_AUDIO_SIMPLE_PLAYER_BIT_CVT_DEST_16BIT
#ifdef CONFIG_AUDIO_SIMPLE_PLAYER_BIT_CVT_DEST_16BIT
  // int bit_depth = 16;
  // int bytes_per_sample = bit_depth / 8;
  int bytes_per_sample = 2;
#endif
#ifdef CONFIG_AUDIO_SIMPLE_PLAYER_BIT_CVT_DEST_24BIT
  // int bit_depth = 24;
  // int bytes_per_sample = bit_depth / 8;
  int bytes_per_sample = 3;
#endif
#ifdef CONFIG_AUDIO_SIMPLE_PLAYER_BIT_CVT_DEST_32BIT
  // int bit_depth = 32;
  // int bytes_per_sample = bit_depth / 8;
  int bytes_per_sample = 4;
#endif

  int bytes_rate = sample_rate * bytes_per_sample;
  float play_time = (float)g_play_size / bytes_rate;

  // ESP_LOGI(TAG, "play_time:%fs", g_play_time);
  return play_time;
}

void mp3_player_set_position(unsigned int position) {}

void mp3_player_deinit() {
  if (player_mutex == NULL) {
    return;
  }
  esp_audio_simple_player_stop(player_handle);
  if (xSemaphoreTake(player_mutex, portMAX_DELAY) == pdTRUE) {
    esp_audio_simple_player_destroy(player_handle);
    xSemaphoreGive(player_mutex);
  }
  vSemaphoreDelete(player_mutex);
}