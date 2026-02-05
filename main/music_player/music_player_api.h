#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MUSIC_PLAYER_STATE_NONE = 0,
    MUSIC_PLAYER_STATE_PLAYING,
    MUSIC_PLAYER_STATE_PAUSED,
    MUSIC_PLAYER_STATE_STOPPED,
    MUSIC_PLAYER_STATE_FINISHED,
    MUSIC_PLAYER_STATE_ERROR
}music_player_state_t;

typedef int (*mp3_player_output_cb_t)(uint8_t *data, int data_size, void *arg);
typedef void (*mp3_player_info_cb_t)(int sample_rate, int channels, int bits, void *arg);
typedef void (*mp3_player_event_cb_t)(music_player_state_t state, void *arg);

void mp3_player_init(mp3_player_output_cb_t output_cb, mp3_player_info_cb_t info_cb, mp3_player_event_cb_t event_cb, void *output_cb_arg);

void mp3_player_play(const char *mp3_path, bool loop, void* ctx);

void mp3_player_pause();

void mp3_player_stop();

void mp3_player_resume();

bool mp3_player_is_playing();

bool mp3_player_is_paused();

bool mp3_player_is_stopped();

bool mp3_player_is_finnished();

unsigned int mp3_player_get_duration();

unsigned int mp3_player_get_position();

void mp3_player_set_position(unsigned int position);

void mp3_player_deinit();

// void mp3_player_set_volume(int volume);

// int mp3_player_get_volume();

// void mp3_player_set_callback(void (*callback)(int event));

#ifdef __cplusplus
}
#endif