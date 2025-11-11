#include "gif_player.h"
#include <esp_log.h>
#include <cstring>
#include <chrono>
#include "display.h"

#define TAG "GifPlayer"

// 静态公共方法
void GifPlayer::CopyAllResourcesToPSRAM() {
    if (!esp_psram_is_initialized()) {
        ESP_LOGE(TAG, "PSRAM not initialized");
        return;
    }

    ESP_LOGI(TAG, "Copying LZ4: %d resources to PSRAM...", lz4_res_count);
    for (int i = 0; i < lz4_res_count; ++i) {
        lz4_res_t* r = (lz4_res_t*)&lz4_res_list[i];
        printf("lz4_res_t name:%s size:%d start:%p psram:%p\n", r->name, (int)r->size, r->start, r->psram);
        uint8_t* dst = (uint8_t*)heap_caps_malloc(r->size, MALLOC_CAP_SPIRAM);
        if (!dst) {
            ESP_LOGE(TAG, "PSRAM malloc failed for %s", r->name);
            continue;
        }
        memcpy(dst, r->start, r->size);
        r->psram = dst;
        ESP_LOGI(TAG, "%s: %d bytes -> PSRAM %p", r->name, (int)r->size, dst);
    }
}

// 公共方法
GifPlayer::GifPlayer(Display* dispaly):display_(dispaly) {
    memset(&img_dsc_, 0, sizeof(img_dsc_));
}

GifPlayer::~GifPlayer() {
    Stop();
    Cleanup();
}

void GifPlayer::InitCanvas(lv_obj_t* content) {
    int img_size = LV_HOR_RES;
    canvas_ = lv_image_create(content);
    lv_obj_set_size(canvas_, img_size, img_size);
    lv_obj_set_style_border_width(canvas_, 0, 0);
    lv_obj_set_style_bg_opa(canvas_, LV_OPA_TRANSP, 0);
    lv_obj_center(canvas_);
}

lz4_res_t* GifPlayer::Getlz4ResByName(const char *emotion) {
    std::string name = lz4_get_gif_name_get_by_name(emotion);
    if (!name.empty()) {
        for (int i = 0; i < lz4_res_count; ++i) {
            if (name == lz4_res_list[i].name) {
                return &lz4_res_list[i];
            }
        }
    }

    ESP_LOGE(TAG, "Unknown emotion: %s for lz4. return null", emotion);
    return nullptr;
}

esp_err_t GifPlayer::LoadAndPlay(const lz4_res_t* res) {
    if (!res || !res->psram) {
        ESP_LOGE(TAG, "Invalid resource");
        return ESP_ERR_INVALID_ARG;
    }

    struct __attribute__((packed)) {
        char magic[4];
        uint16_t width;
        uint16_t height;
        uint8_t  fps;
        uint8_t  reserved;
        uint16_t frames;
        uint32_t data_offset;
    } hdr;
    
    memcpy(&hdr, res->psram, sizeof(hdr));
    if (memcmp(hdr.magic, "GIFL", 4) != 0) {
        ESP_LOGE(TAG, "Invalid magic: %.4s", hdr.magic);
        return ESP_ERR_INVALID_ARG;
    }

    if (current_emotion_ == res->name && first_frame_) {
        ESP_LOGI(TAG, "Same GIF already playing: %s", res->name);
        return ESP_OK; // Already playing this GIF
    }

    ESP_LOGI(TAG, "Loading GIF: %s. release GIF: %s", res->name, current_emotion_.c_str());

    Cleanup();
    current_emotion_ = res->name;

    uint32_t frame_size = hdr.width * hdr.height * 2;
    if (!frame_buffer_) {
        frame_buffer_ = (uint8_t*)heap_caps_malloc(frame_size, MALLOC_CAP_SPIRAM);
        if (!frame_buffer_) {
            ESP_LOGE(TAG, "Failed to allocate frame buffer");
            return ESP_ERR_NO_MEM;
        }
    }

    if (!old_frame_buffer_) {
        old_frame_buffer_ = (uint8_t*)heap_caps_malloc(frame_size, MALLOC_CAP_SPIRAM);
        if (!old_frame_buffer_) {
            ESP_LOGE(TAG, "Failed to allocate frame buffer");
            return ESP_ERR_NO_MEM;
        }
    }

    width_ = hdr.width;
    height_ = hdr.height;
    fps_ = hdr.fps;
    total_frames_ = hdr.frames;
    current_frame_ = 0;
    lz4_data_ = res->psram + hdr.data_offset;
    lz4_size_ = res->size - hdr.data_offset;

    img_dsc_ = {}; //这一步很重要，确保结构体清零
    img_dsc_.header.w = width_;
    img_dsc_.header.h = height_;
    img_dsc_.header.cf = LV_COLOR_FORMAT_RGB565;
    img_dsc_.data_size = frame_size;
    img_dsc_.data = frame_buffer_;

    ESP_LOGI(TAG, "Loaded GIF:%s %dx%d, %d frames, %d FPS", 
            res->name, width_, height_, total_frames_, fps_);
    
    Play();

    return ESP_OK;
}

esp_err_t GifPlayer::Play() {
    if (total_frames_ == 0) {
        ESP_LOGE(TAG, "No GIF loaded");
        return ESP_ERR_INVALID_STATE;
    }

    if (!timer_handle_) {
        SetFrame(0);

        esp_timer_create_args_t clock_timer_args = {
            .callback = [](void* arg) {
                GifPlayer* player = (GifPlayer*)arg;
                player->OnTimer();
            },
            .arg = this,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "gif_timer",
            .skip_unhandled_events = true
        };
        esp_timer_create(&clock_timer_args, &timer_handle_);
    }
    int interval_ms = 1000;
    if (timer_handle_) {
        if (fps_ > 0) {
            interval_ms = (1000 / fps_ + 11) / 10 * 10 - 10;
        }
        interval_ms = std::max(interval_ms, 10);
        esp_timer_start_periodic(timer_handle_, interval_ms * 1000);
    }

    ESP_LOGI(TAG, "Started playing GIF %d", interval_ms);
    return ESP_OK;
}

esp_err_t GifPlayer::Stop() {    
    if (timer_handle_) {
        esp_timer_stop(timer_handle_);
    }
    if (canvas_) {
        lv_obj_del(canvas_);
        canvas_ = nullptr;
    }
    
    ESP_LOGI(TAG, "Stopped playing GIF");
    return ESP_OK;
}

bool get_diff_area(const uint8_t* __restrict new_,
                   const uint8_t* __restrict old_,
                   lv_area_t* dirty,
                   int img_w, int img_h)
{
    constexpr int W   = 240;
    constexpr int H   = 240;
    constexpr int rowBytes = W * 2;          // 480 字节/行

    int x0 = W, y0 = H, x1 = -1, y1 = -1;

    /* 按行扫描，每 4 字节一组比较 */
    for (int y = 0; y < H; ++y) {
        const uint32_t* nPtr = reinterpret_cast<const uint32_t*>(new_ + y * rowBytes);
        const uint32_t* oPtr = reinterpret_cast<const uint32_t*>(old_  + y * rowBytes);
        int w32 = rowBytes / 4;              // 120 组
        int firstDiff32 = -1, lastDiff32 = -1;

        for (int i = 0; i < w32; ++i) {
            if (nPtr[i] != oPtr[i]) {
                if (firstDiff32 < 0) firstDiff32 = i;
                lastDiff32 = i;
            }
        }

        if (firstDiff32 >= 0) {              // 本行有变化
            int rowX0 = firstDiff32 * 4 / 2; // 转回像素坐标
            int rowX1 = (lastDiff32 + 1) * 4 / 2 - 1;
            x0 = std::min(x0, rowX0);
            x1 = std::max(x1, rowX1);
            y0 = std::min(y0, y);
            y1 = std::max(y1, y);
        }
    }

    if (x1 < 0) return false;                // 全帧相同

    dirty->x1 = x0;
    dirty->y1 = y0;
    dirty->x2 = x1;
    dirty->y2 = y1;
    return true;
}

esp_err_t GifPlayer::SetFrame(int frame_index) {
    esp_err_t ret = DecodeFrame(frame_index);
    if (ret == ESP_OK && canvas_) {
        DisplayLockGuard lock(display_);
        if (!first_frame_) {
            lv_image_set_src(canvas_, &img_dsc_);
            first_frame_ = true;
        } else if (diff_redraw_) {
            lv_area_t dirty;
            if (get_diff_area(old_frame_buffer_, frame_buffer_, &dirty, img_dsc_.header.w, img_dsc_.header.h)) {
                lv_obj_invalidate_area(canvas_, &dirty);  
            }
        } else {
            lv_obj_invalidate(canvas_);
        }
    }
    return ret;
}

// 私有方法实现
esp_err_t GifPlayer::DecodeFrame(int frame_index) {
    if (frame_index < 0 || frame_index >= total_frames_) {
        ESP_LOGE(TAG, "Frame index out of range: %d", frame_index);
        return ESP_ERR_INVALID_ARG;
    }

    if (!lz4_data_ || !frame_buffer_) {
        ESP_LOGE(TAG, "GIF not loaded properly");
        return ESP_ERR_INVALID_STATE;
    }

    const uint8_t* p = lz4_data_;
    for (int i = 0; i < frame_index; ++i) {
        uint32_t len = *(uint32_t*)p;
        p += 4 + len;
    }

    uint32_t compressed_len = *(uint32_t*)p;
    p += 4;
    
    memcpy(old_frame_buffer_, frame_buffer_, width_ * height_ * 2);

    int decompressed_size = width_ * height_ * 2;
    int result = LZ4_decompress_safe((const char*)p, (char*)frame_buffer_, 
                                    compressed_len, decompressed_size);

    if (result == decompressed_size) {
        current_frame_ = frame_index;
        ESP_LOGD(TAG, "Decoded frame %d/%d", frame_index + 1, total_frames_);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "LZ4 decompression failed: expected %d, got %d", 
                 decompressed_size, result);
        return ESP_ERR_INVALID_SIZE;
    }
}

void GifPlayer::Cleanup() {
    if (timer_handle_) {
        esp_timer_stop(timer_handle_);
    }
    // if (frame_buffer_) {
    //     heap_caps_free(frame_buffer_);
    //     frame_buffer_ = nullptr;
    // }
    
    lz4_data_ = nullptr;
    lz4_size_ = 0;
    
    current_emotion_.clear();
    width_ = 0;
    height_ = 0;
    fps_ = 0;
    total_frames_ = 0;
    current_frame_ = 0;
    
    // memset(&img_dsc_, 0, sizeof(img_dsc_));
}

void GifPlayer::OnTimer() {
    int next_frame = (current_frame_ + 1) % total_frames_;
    SetFrame(next_frame);
}