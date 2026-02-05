#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H 
#include "music_player_api.h"
#include "audio/audio_codec.h"
#include "audio/audio_service.h"
#include <string>
#include <vector>
#include "esp_log.h"
#include "mp3_online_player.h"
struct MusicInfo {
    std::string name;
    std::string uri;
    std::string album;
    std::string artist;
};

class MusicListManager {
public:
    MusicListManager() {}
    ~MusicListManager() {}

    void SetAirMusicList(const std::vector<MusicInfo>& music_list) {
        air_music_list_ = music_list;
        if (!air_music_list_.empty()) {
            current_air_music_index_ = 0;
        } else {
            current_air_music_index_ = -1;
        }
    }

    void CacheAirMusicList(const std::vector<MusicInfo>& music_list) {
        air_music_list_cache_ = music_list;
    }

    void ClearCacheAirMusicList() {
        air_music_list_cache_.clear();
    }

    bool HasCachedAirMusicList() {
       return air_music_list_cache_.size() > 0;
    }

    bool PlayCachedAirMusicByIndex(int index) {
        if (air_music_list_cache_.empty()) {
            return false;
        }
        if(index < 0 || index >= static_cast<int>(air_music_list_cache_.size())){
            index = 0;
        }
        air_music_list_.clear();
        air_music_list_.push_back(air_music_list_cache_[index]);
        current_air_music_index_ = 0;
        air_music_list_cache_.clear();
        return true;
    }

    void SetLocalMusicList(const std::vector<MusicInfo>& music_list) {
        local_music_list_ = music_list;
        if (!local_music_list_.empty()) {
            current_local_music_index_ = 0;
        } else {
            current_local_music_index_ = -1;
        }
    }

    void InitLocalMusicListAuto(const std::string& sd_card_music_path) {
        // to do
    }

    void ClearAirMusicList() {
        air_music_list_.clear();
        current_air_music_index_ = -1;
    }

    void ClearLocalMusicList() {
        local_music_list_.clear();
        current_local_music_index_ = -1;
    }

    MusicInfo GetAirMusicByIndex(int index) {
        if (air_music_list_.empty() || index < 0 || index >= static_cast<int>(air_music_list_.size())) {
            return {};
        }
        current_air_music_index_ = index;
        return air_music_list_[current_air_music_index_];
    }
    bool IsAirMusicListEmpty() {
        return air_music_list_.empty();
    }
    MusicInfo GetCurrentAirMusic() {
        if (air_music_list_.empty() || current_air_music_index_ == -1) {
            return {};
        }
        return air_music_list_[current_air_music_index_];
    }

    MusicInfo GetCurrentLocalMusic() {
        if (local_music_list_.empty() || current_local_music_index_ == -1) {
            return {};
        }
        return local_music_list_[current_local_music_index_];
    }

    MusicInfo GetPreviousAirMusic() {
        if (air_music_list_.empty()) {
            return {};
        }
        current_air_music_index_ = (current_air_music_index_ - 1 + air_music_list_.size()) % air_music_list_.size();
        return air_music_list_[current_air_music_index_];
    }

    MusicInfo GetNextAirMusic() {
        if (air_music_list_.empty()) {
            return {};
        }
        current_air_music_index_ = (current_air_music_index_ + 1) % air_music_list_.size();
        return air_music_list_[current_air_music_index_];
    }

    MusicInfo GetNextLocalMusic() {
        if (local_music_list_.empty()) {
            return {};
        }
        current_local_music_index_ = (current_local_music_index_ + 1) % local_music_list_.size();
        return local_music_list_[current_local_music_index_];
    }

    bool CurrentLastMusicOnList(bool is_air_music) {
        if (air_music_list_.empty() && is_air_music) {
            return true;
        }
        if (local_music_list_.empty() && !is_air_music) {
            return true;
        }
        if (is_air_music) {
            return current_air_music_index_ == static_cast<int>(air_music_list_.size()) - 1;
        } else {
            return current_local_music_index_ == static_cast<int>(local_music_list_.size()) - 1;
        }
        return true;
    }

private:
    std::vector<MusicInfo> air_music_list_;
    std::vector<MusicInfo> local_music_list_;
    std::vector<MusicInfo> air_music_list_cache_;
    int current_air_music_index_ = -1;
    int current_local_music_index_ = -1;
};

//music player 单例
class MusicPlayer {
public:
    static MusicPlayer& GetInstance() {
        static MusicPlayer instance;
        return instance;
    }
public:
    int Initialize(AudioCodec* codec, AudioService* audio_service, std::string sd_card_music_path = "");
    bool PlayAirMusicByIndex(int index){
        MusicInfo music = music_list_manager_.GetAirMusicByIndex(index);
        if(!music.uri.empty()){
            Play(music.uri.c_str());
            return true;
        }
        return false;
    }
    bool PlayCurrentAirMusic(){
        MusicInfo music = music_list_manager_.GetCurrentAirMusic();
        if(!music.uri.empty()){
            Play(music.uri.c_str());
            return true;
        }
        return false;
    }
    bool PlayNextAirMusic(){
        MusicInfo music = music_list_manager_.GetNextAirMusic();
        if(!music.uri.empty()){
            Play(music.uri.c_str());
            return true;
        }
        return false;
    }
    bool PlayPreviousAirMusic(){
        MusicInfo music = music_list_manager_.GetPreviousAirMusic();
        if(!music.uri.empty()){
            Play(music.uri.c_str());
            return true;
        }
        return false;
    }

    void UpdateAirMusicListAndPlay(const std::vector<MusicInfo>& music_list, bool play_now);

    void StopAirPlay(){
        mp3_online_player_.StopStreaming();
    }
    void StopPlay() {
        mp3_player_stop();
    }

    bool CurrentPlayingLastMusic(){
        return music_list_manager_.CurrentLastMusicOnList(is_air_music_playing_);
    }
public:
    int DataCallback(uint8_t *data, int data_size);
    void InfoCallback(int sample_rate, int channels, int bits);
    void PlayStateCallback(music_player_state_t state);
    void InterruptPlay(){
        if(!initialed){
            return;
        }
        StopAirPlay();
        if(current_state_ == music_player_state_t::MUSIC_PLAYER_STATE_PLAYING){
            ESP_LOGI("MusicPlayer", "InterruptPlay: stopping current music playback");
            StopPlay();
        }
    }
private:
    MusicPlayer();
    ~MusicPlayer() {}
    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer& operator=(const MusicPlayer&) = delete;
private:
    void Play(const char* mp3_path);  
private:
    int sample_rate_ = 48000;
    int channels_ = 2;
    int bits_ = 16;
    AudioCodec* codec_ = nullptr;
    MusicListManager music_list_manager_;
    AudioService* audio_service_;
    music_player_state_t current_state_ = music_player_state_t::MUSIC_PLAYER_STATE_NONE;
    bool is_air_music_playing_ = true;
    std::mutex call_back_mutex_;
    bool initialed = false;
    Mp3OnlinePlayer mp3_online_player_;
};
#endif //MUSIC_PLAYER_H