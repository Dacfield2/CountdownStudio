#include "countdown-source.hpp"

#include "countdownstudio/countdown.hpp"

#include <graphics/graphics.h>
#include <obs-data.h>
#include <obs-module.h>
#include <obs-properties.h>
#include <obs-source.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace countdownstudio {
namespace {

constexpr uint32_t kCanvasWidth = 1280;
constexpr uint32_t kCanvasHeight = 360;
constexpr int kMargin = 32;
constexpr int64_t kDefaultDurationSeconds = 600;
constexpr int64_t kMaximumDurationSeconds = 604800;
constexpr std::size_t kChannels = 4;
constexpr uint32_t kDefaultTextColor = 0xFFFFFFFF;
constexpr uint32_t kDefaultBackgroundColor = 0x99000000;

using Glyph = std::array<uint8_t, 7>;

Glyph glyph_for(char character)
{
    switch (character) {
    case '0': return {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110};
    case '1': return {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110};
    case '2': return {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111};
    case '3': return {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110};
    case '4': return {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010};
    case '5': return {0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110};
    case '6': return {0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110};
    case '7': return {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000};
    case '8': return {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110};
    case '9': return {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110};
    case ':': return {0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000};
    default: return {};
    }
}

int64_t read_duration(obs_data_t *settings)
{
    return std::clamp<int64_t>(obs_data_get_int(settings, "duration_seconds"), 1,
                               kMaximumDurationSeconds);
}

std::string format_remaining(Countdown::Duration remaining, int format)
{
    const auto whole_seconds = std::chrono::ceil<std::chrono::seconds>(remaining).count();
    const auto hours = whole_seconds / 3600;
    const auto minutes = (whole_seconds / 60) % 60;
    const auto seconds = whole_seconds % 60;

    std::ostringstream text;
    text << std::setfill('0');
    if (format == 1 || (format == 0 && hours == 0)) {
        text << std::setw(2) << (whole_seconds / 60) << ':' << std::setw(2) << seconds;
    } else {
        text << std::setw(2) << hours << ':' << std::setw(2) << minutes << ':' << std::setw(2) << seconds;
    }
    return text.str();
}

int64_t display_seconds(Countdown::Duration remaining)
{
    return std::chrono::ceil<std::chrono::seconds>(remaining).count();
}

class CountdownSource final {
public:
    explicit CountdownSource(obs_data_t *settings)
        : duration_seconds_(read_duration(settings)),
          auto_start_(obs_data_get_bool(settings, "auto_start")),
          show_background_(obs_data_get_bool(settings, "show_background")),
          text_color_(static_cast<uint32_t>(obs_data_get_int(settings, "text_color"))),
          background_color_(static_cast<uint32_t>(obs_data_get_int(settings, "background_color"))),
          font_size_percent_(static_cast<int>(obs_data_get_int(settings, "font_size_percent"))),
          time_format_(static_cast<int>(obs_data_get_int(settings, "time_format"))),
          timer_(std::chrono::duration_cast<Countdown::Duration>(std::chrono::seconds{duration_seconds_})),
          pixels_(static_cast<std::size_t>(kCanvasWidth) * kCanvasHeight * kChannels, 0)
    {
        if (auto_start_) {
            timer_.start();
        }
        render_pixels_locked();
    }

    ~CountdownSource()
    {
        if (texture_ != nullptr) {
            obs_enter_graphics();
            gs_texture_destroy(texture_);
            obs_leave_graphics();
        }
    }

    void update(obs_data_t *settings)
    {
        std::lock_guard lock(mutex_);
        const auto new_duration = read_duration(settings);
        const auto new_auto_start = obs_data_get_bool(settings, "auto_start");
        show_background_ = obs_data_get_bool(settings, "show_background");
        text_color_ = static_cast<uint32_t>(obs_data_get_int(settings, "text_color"));
        background_color_ = static_cast<uint32_t>(obs_data_get_int(settings, "background_color"));
        font_size_percent_ = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "font_size_percent"), 25, 100));
        time_format_ = static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "time_format"), 0, 2));

        if (new_duration != duration_seconds_ || new_auto_start != auto_start_) {
            duration_seconds_ = new_duration;
            auto_start_ = new_auto_start;
            reset_timer_locked(auto_start_);
        } else {
            render_pixels_locked();
        }
    }

    void tick()
    {
        std::lock_guard lock(mutex_);
        if (display_seconds(timer_.remaining()) != last_display_seconds_) {
            render_pixels_locked();
        }
    }

    void render()
    {
        std::lock_guard lock(mutex_);
        if (texture_ == nullptr) {
            texture_ = gs_texture_create(kCanvasWidth, kCanvasHeight, GS_RGBA, 1, nullptr, GS_DYNAMIC);
            if (texture_ == nullptr) {
                return;
            }
            texture_dirty_ = true;
        }

        if (texture_dirty_) {
            gs_texture_set_image(texture_, pixels_.data(), kCanvasWidth * static_cast<uint32_t>(kChannels), false);
            texture_dirty_ = false;
        }

        obs_source_draw(texture_, 0, 0, kCanvasWidth, kCanvasHeight, false);
    }

    void start()
    {
        std::lock_guard lock(mutex_);
        timer_.start();
        render_pixels_locked();
    }

    void pause()
    {
        std::lock_guard lock(mutex_);
        timer_.pause();
        render_pixels_locked();
    }

    void add_minute()
    {
        add_time(std::chrono::minutes{1});
    }

    void add_five_minutes()
    {
        add_time(std::chrono::minutes{5});
    }

    void restart()
    {
        std::lock_guard lock(mutex_);
        reset_timer_locked(true);
    }

private:
    void add_time(Countdown::Duration extra)
    {
        std::lock_guard lock(mutex_);
        timer_.add_time(extra);
        render_pixels_locked();
    }

    void reset_timer_locked(bool start)
    {
        timer_.reset(std::chrono::duration_cast<Countdown::Duration>(std::chrono::seconds{duration_seconds_}));
        if (start) {
            timer_.start();
        }
        render_pixels_locked();
    }

    void set_pixel_block(int x, int y, int scale, uint32_t color)
    {
        const auto red = static_cast<uint8_t>((color >> 16) & 0xFF);
        const auto green = static_cast<uint8_t>((color >> 8) & 0xFF);
        const auto blue = static_cast<uint8_t>(color & 0xFF);
        const auto alpha = static_cast<uint8_t>((color >> 24) & 0xFF);
        for (int row = 0; row < scale; ++row) {
            for (int column = 0; column < scale; ++column) {
                const auto pixel_x = static_cast<std::size_t>(x + column);
                const auto pixel_y = static_cast<std::size_t>(y + row);
                const auto offset = (pixel_y * kCanvasWidth + pixel_x) * kChannels;
                pixels_[offset] = red;
                pixels_[offset + 1] = green;
                pixels_[offset + 2] = blue;
                pixels_[offset + 3] = alpha;
            }
        }
    }

    void render_pixels_locked()
    {
        std::fill(pixels_.begin(), pixels_.end(), 0);
        if (show_background_) {
            const auto red = static_cast<uint8_t>((background_color_ >> 16) & 0xFF);
            const auto green = static_cast<uint8_t>((background_color_ >> 8) & 0xFF);
            const auto blue = static_cast<uint8_t>(background_color_ & 0xFF);
            const auto alpha = static_cast<uint8_t>((background_color_ >> 24) & 0xFF);
            for (std::size_t offset = 0; offset < pixels_.size(); offset += kChannels) {
                pixels_[offset] = red;
                pixels_[offset + 1] = green;
                pixels_[offset + 2] = blue;
                pixels_[offset + 3] = alpha;
            }
        }

        const auto text = format_remaining(timer_.remaining(), time_format_);
        const int text_units = static_cast<int>(text.size()) * 6 - 1;
        const int scale_x = (static_cast<int>(kCanvasWidth) - 2 * kMargin) / text_units;
        const int scale_y = (static_cast<int>(kCanvasHeight) - 2 * kMargin) / 7;
        const int scale = std::max(1, std::min(scale_x, scale_y) * font_size_percent_ / 100);
        const int text_width = text_units * scale;
        const int origin_x = (static_cast<int>(kCanvasWidth) - text_width) / 2;
        const int origin_y = (static_cast<int>(kCanvasHeight) - 7 * scale) / 2;

        for (std::size_t index = 0; index < text.size(); ++index) {
            const auto glyph = glyph_for(text[index]);
            const int glyph_x = origin_x + static_cast<int>(index) * 6 * scale;
            for (std::size_t row = 0; row < glyph.size(); ++row) {
                for (int column = 0; column < 5; ++column) {
                    if ((glyph[row] & (1U << (4 - column))) != 0) {
                        set_pixel_block(glyph_x + column * scale, origin_y + static_cast<int>(row) * scale, scale,
                                        text_color_);
                    }
                }
            }
        }

        last_display_seconds_ = display_seconds(timer_.remaining());
        texture_dirty_ = true;
    }

    int64_t duration_seconds_;
    bool auto_start_;
    bool show_background_;
    uint32_t text_color_;
    uint32_t background_color_;
    int font_size_percent_;
    int time_format_;
    Countdown timer_;
    std::vector<uint8_t> pixels_;
    int64_t last_display_seconds_{std::numeric_limits<int64_t>::min()};
    gs_texture_t *texture_{nullptr};
    bool texture_dirty_{true};
    std::mutex mutex_;
};

void *source_create(obs_data_t *settings, obs_source_t *)
{
    return new CountdownSource(settings);
}

void source_destroy(void *data)
{
    delete static_cast<CountdownSource *>(data);
}

void source_update(void *data, obs_data_t *settings)
{
    static_cast<CountdownSource *>(data)->update(settings);
}

void source_tick(void *data, float)
{
    static_cast<CountdownSource *>(data)->tick();
}

void source_render(void *data, gs_effect_t *)
{
    static_cast<CountdownSource *>(data)->render();
}

uint32_t source_width(void *)
{
    return kCanvasWidth;
}

uint32_t source_height(void *)
{
    return kCanvasHeight;
}

const char *source_name(void *)
{
    return obs_module_text("Source.Name");
}

void source_defaults(obs_data_t *settings)
{
    obs_data_set_default_int(settings, "duration_seconds", kDefaultDurationSeconds);
    obs_data_set_default_bool(settings, "auto_start", true);
    obs_data_set_default_bool(settings, "show_background", false);
    obs_data_set_default_int(settings, "text_color", kDefaultTextColor);
    obs_data_set_default_int(settings, "background_color", kDefaultBackgroundColor);
    obs_data_set_default_int(settings, "font_size_percent", 100);
    obs_data_set_default_int(settings, "time_format", 0);
}

bool start_clicked(obs_properties_t *, obs_property_t *, void *data)
{
    static_cast<CountdownSource *>(data)->start();
    return false;
}

bool pause_clicked(obs_properties_t *, obs_property_t *, void *data)
{
    static_cast<CountdownSource *>(data)->pause();
    return false;
}

bool restart_clicked(obs_properties_t *, obs_property_t *, void *data)
{
    static_cast<CountdownSource *>(data)->restart();
    return false;
}

bool add_minute_clicked(obs_properties_t *, obs_property_t *, void *data)
{
    static_cast<CountdownSource *>(data)->add_minute();
    return false;
}

bool add_five_minutes_clicked(obs_properties_t *, obs_property_t *, void *data)
{
    static_cast<CountdownSource *>(data)->add_five_minutes();
    return false;
}

obs_properties_t *source_properties(void *data)
{
    auto *properties = obs_properties_create();
    obs_properties_add_int(properties, "duration_seconds", obs_module_text("Settings.Duration"), 1,
                           static_cast<int>(kMaximumDurationSeconds), 1);
    obs_properties_add_bool(properties, "auto_start", obs_module_text("Settings.AutoStart"));
    obs_properties_add_bool(properties, "show_background", obs_module_text("Settings.ShowBackground"));
    obs_properties_add_color_alpha(properties, "text_color", obs_module_text("Settings.TextColor"));
    obs_properties_add_color_alpha(properties, "background_color", obs_module_text("Settings.BackgroundColor"));
    obs_properties_add_int_slider(properties, "font_size_percent", obs_module_text("Settings.FontSize"), 25, 100, 1);
    auto *format = obs_properties_add_list(properties, "time_format", obs_module_text("Settings.TimeFormat"),
                                           OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(format, obs_module_text("Format.Automatic"), 0);
    obs_property_list_add_int(format, "MM:SS", 1);
    obs_property_list_add_int(format, "HH:MM:SS", 2);

    if (data != nullptr) {
        obs_properties_add_button2(properties, "start", obs_module_text("Controls.Start"), start_clicked, data);
        obs_properties_add_button2(properties, "pause", obs_module_text("Controls.Pause"), pause_clicked, data);
        obs_properties_add_button2(properties, "restart", obs_module_text("Controls.Restart"), restart_clicked, data);
        obs_properties_add_button2(properties, "add_minute", obs_module_text("Controls.AddMinute"),
                                   add_minute_clicked, data);
        obs_properties_add_button2(properties, "add_five_minutes", obs_module_text("Controls.AddFiveMinutes"),
                                   add_five_minutes_clicked, data);
    }
    return properties;
}

} // namespace

void register_countdown_source()
{
    static obs_source_info source{};
    source.id = "countdown_studio_timer";
    source.type = OBS_SOURCE_TYPE_INPUT;
    source.output_flags = OBS_SOURCE_VIDEO;
    source.get_name = source_name;
    source.create = source_create;
    source.destroy = source_destroy;
    source.update = source_update;
    source.get_defaults = source_defaults;
    source.get_properties = source_properties;
    source.video_tick = source_tick;
    source.video_render = source_render;
    source.get_width = source_width;
    source.get_height = source_height;
    obs_register_source(&source);
}

} // namespace countdownstudio
