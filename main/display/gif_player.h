#pragma once

#include <stdint.h>
#include <string>
#include "lz4.h"
#include <lvgl.h>
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include <esp_timer.h>
#include "lz4_auto.h"

class Display;
class GifPlayer {
public:
    // 构造函数/析构函数
    GifPlayer(Display* dispaly); //传递 display 主要是为了使用锁
    ~GifPlayer();

    // 静态方法：加载所有资源到PSRAM
    static void CopyAllResourcesToPSRAM();

    // 公共接口方法
    void      InitCanvas(lv_obj_t* content);
    void      SetDiffRedraw(bool diff_redraw) { diff_redraw_ = diff_redraw; }
    lz4_res_t *Getlz4ResByName(const char *emotion);
    esp_err_t LoadAndPlay(const lz4_res_t* res);
    
    // 获取信息（只读）
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetTotalFrames() const { return total_frames_; }
    int GetCurrentFrame() const { return current_frame_; }
    int GetFPS() const { return fps_; }
    bool IsPlaying() const { return first_frame_; }
    
    // 获取LVGL对象（只读）
    lv_obj_t* GetCanvas() const { return canvas_; }
    const lv_img_dsc_t* GetImageDescriptor() const { return &img_dsc_; }

private:
    // 私有方法：不对外暴露
    esp_err_t Play();
    esp_err_t Stop();
    esp_err_t SetFrame(int frame_index);
    esp_err_t DecodeFrame(int frame_index);
    void Cleanup();
    void OnTimer();

    // 成员变量
    Display* display_ = nullptr; // 用于锁定显示
    const uint8_t* lz4_data_ = nullptr;
    uint32_t lz4_size_ = 0;
    uint8_t* frame_buffer_ = nullptr;
    uint8_t* old_frame_buffer_ = nullptr;
    std::string current_emotion_;
    int width_ = 0;
    int height_ = 0;
    int fps_ = 0;
    int total_frames_ = 0;
    int current_frame_ = 0;
    bool first_frame_ = false;
    bool diff_redraw_ = true;
    
    lv_img_dsc_t img_dsc_ = {};
    lv_obj_t* canvas_ = nullptr;
    esp_timer_handle_t timer_handle_ = nullptr;

    // 禁用拷贝
    GifPlayer(const GifPlayer&) = delete;
    GifPlayer& operator=(const GifPlayer&) = delete;
};