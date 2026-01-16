#ifndef _AUDIO_CODEC_H
#define _AUDIO_CODEC_H

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <driver/i2s_std.h>

#include <vector>
#include <string>
#include <functional>

#include "board.h"

#define AUDIO_CODEC_DMA_DESC_NUM 6
#define AUDIO_CODEC_DMA_FRAME_NUM 240

class AudioCodec {
public:
    AudioCodec();
    virtual ~AudioCodec();
    
    virtual void SetOutputVolume(int volume);
    virtual void SetInputGain(float gain);
    virtual void EnableInput(bool enable);
    virtual void EnableOutput(bool enable);

    virtual void OutputData(std::vector<int16_t>& data);
    virtual bool InputData(std::vector<int16_t>& data);
#ifdef CONFIG_USE_AUDIO_CODEC_ENCODE_OPUS
    virtual bool InputData(std::vector<uint8_t>& opus);
    virtual bool ConfigEncode(int sample_rate, int channels, int duration_ms);
#endif
    virtual void Start();
    virtual void SetOpusFrameDuration(int duration);

    inline bool duplex() const { return duplex_; }
    inline bool input_reference() const { return input_reference_; }
    inline int input_sample_rate() const { return input_sample_rate_; }
    inline int output_sample_rate() const { return output_sample_rate_; }
    inline int input_channels() const { return input_channels_; }
    inline int output_channels() const { return output_channels_; }
    inline int output_volume() const { return output_volume_; }
    inline float input_gain() const { return input_gain_; }
    inline bool input_enabled() const { return input_enabled_; }
    inline bool output_enabled() const { return output_enabled_; }
    inline int opus_frame_duration() const { return opus_frame_duration_; }
#ifdef CONFIG_USE_AUDIO_CODEC_ENCODE_OPUS
    inline bool input_duration_ms() const { return input_duration_ms_; }
#endif


protected:
    i2s_chan_handle_t tx_handle_ = nullptr;
    i2s_chan_handle_t rx_handle_ = nullptr;

    bool duplex_ = false;
    bool input_reference_ = false;
    bool input_enabled_ = false;
    bool output_enabled_ = false;
    int input_sample_rate_ = 0;
    int output_sample_rate_ = 0;
    int input_channels_ = 1;
    int output_channels_ = 1;
    int output_volume_ = 70;
    float input_gain_ = 0.0;
    int opus_frame_duration_ = 60;
#ifdef CONFIG_USE_AUDIO_CODEC_ENCODE_OPUS
    int input_duration_ms_ = 60;
#endif

    virtual int Read(int16_t* dest, int samples) = 0;
    virtual int Write(const int16_t* data, int samples) = 0;
#ifdef CONFIG_USE_AUDIO_CODEC_ENCODE_OPUS
    virtual int Read(uint8_t* opus, int samples) = 0;
#endif
};

#endif // _AUDIO_CODEC_H
