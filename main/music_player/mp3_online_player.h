#ifndef MP3_ONLINE_PLAYER
#define MP3_ONLINE_PLAYER
#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include "music_player_api.h"

struct AudioChunk {
    uint8_t* data;
    size_t size;
    
    AudioChunk() : data(nullptr), size(0) {}
    AudioChunk(uint8_t* d, size_t s) : data(d), size(s) {}
};

// MP3解码器支持
extern "C" {
#include "mp3dec.h"
}
class Mp3OnlinePlayer{
public:
    Mp3OnlinePlayer();
    ~Mp3OnlinePlayer();
public:
    bool StartStreaming(const std::string& music_url);
    bool StopStreaming();  // 停止流式播放
    bool IsPlaying() const { return is_playing_; }
    void Mp3OnlinePlayerInit(mp3_player_output_cb_t output_cb, mp3_player_info_cb_t info_cb, mp3_player_event_cb_t event_cb, void *output_cb_arg);
private:
    // 私有方法
    void DownloadAudioStream(const std::string& music_url);
    void PlayAudioStream();
    void ClearAudioBuffer();
    bool InitializeMp3Decoder();
    void CleanupMp3Decoder();

    // ID3标签处理
    size_t SkipId3Tag(uint8_t* data, size_t size);
private:
    std::atomic<bool> is_playing_;
    std::atomic<bool> is_downloading_;
    std::thread play_thread_;
    std::thread download_thread_;
    int64_t current_play_time_ms_;  // 当前播放时间(毫秒)
    int64_t last_frame_time_ms_;    // 上一帧的时间戳
    int total_frames_decoded_;      // 已解码的帧数
    mp3_player_output_cb_t output_cb_ = nullptr;
    mp3_player_info_cb_t info_cb_ = nullptr;
    mp3_player_event_cb_t event_cb_ = nullptr;
    std::atomic<bool> need_info_cb_ = false;
    music_player_state_t player_state_ = music_player_state_t::MUSIC_PLAYER_STATE_NONE;
    void* user_context_;
    // 音频缓冲区
    std::queue<AudioChunk> audio_buffer_;
    std::mutex buffer_mutex_;
    std::condition_variable buffer_cv_;
    std::mutex thread_control_mutex_;
    size_t buffer_size_;
    static constexpr size_t MAX_BUFFER_SIZE = 256 * 1024;  // 256KB缓冲区（降低以减少brownout风险）
    static constexpr size_t MIN_BUFFER_SIZE = 32 * 1024;   // 32KB最小播放缓冲（降低以减少brownout风险）

    // MP3解码器相关
    HMP3Decoder mp3_decoder_;
    MP3FrameInfo mp3_frame_info_;
    bool mp3_decoder_initialized_;
    int16_t* final_pcm_data_fft = nullptr;

};
#endif