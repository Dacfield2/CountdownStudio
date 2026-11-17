#include "countdown-source.hpp"

#include "countdownstudio/countdown.hpp"

#include <graphics/graphics.h>
#include <obs-data.h>
#include <obs-module.h>
#include <obs-properties.h>
#include <obs-source.h>

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QString>

#include <algorithm>
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
          font_size_percent_(
              static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "font_size_percent"), 25, 100))),
          time_format_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "time_format"), 0, 2))),
          timer_(std::chrono::duration_cast<Countdown::Duration>(std::chrono::seconds{duration_seconds_})),
          pixels_(static_cast<std::size_t>(kCanvasWidth) * kCanvasHeight * kChannels, 0)
    {
        read_font_settings(settings);
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
        read_font_settings(settings);

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

    void read_font_settings(obs_data_t *settings)
    {
        obs_data_t *font = obs_data_get_obj(settings, "font");
        if (font == nullptr) {
            return;
        }

        const char *face = obs_data_get_string(font, "face");
        const char *style = obs_data_get_string(font, "style");
        if (face != nullptr && face[0] != '\0') {
            font_face_ = face;
        }
        if (style != nullptr) {
            font_style_ = style;
        }
        font_size_points_ = static_cast<int>(std::clamp<int64_t>(obs_data_get_int(font, "size"), 1, 1000));
        font_flags_ = static_cast<uint32_t>(obs_data_get_int(font, "flags"));
        obs_data_release(font);
    }

    void render_pixels_locked()
    {
        const auto text = format_remaining(timer_.remaining(), time_format_);
        QImage image(pixels_.data(), static_cast<int>(kCanvasWidth), static_cast<int>(kCanvasHeight),
                     static_cast<int>(kCanvasWidth * kChannels), QImage::Format_RGBA8888);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        if (show_background_) {
            painter.fillRect(image.rect(), QColor::fromRgba(background_color_));
        }

        QFont font(QString::fromUtf8(font_face_.c_str()));
        font.setStyleName(QString::fromUtf8(font_style_.c_str()));
        font.setBold((font_flags_ & OBS_FONT_BOLD) != 0);
        font.setItalic((font_flags_ & OBS_FONT_ITALIC) != 0);
        font.setUnderline((font_flags_ & OBS_FONT_UNDERLINE) != 0);
        font.setStrikeOut((font_flags_ & OBS_FONT_STRIKEOUT) != 0);
        font.setPointSize(font_size_points_);

        const QString display_text = QString::fromStdString(text);
        const QFontMetricsF initial_metrics(font);
        const QRectF initial_bounds = initial_metrics.tightBoundingRect(display_text);
        const double width_limit = static_cast<double>(kCanvasWidth - 2 * kMargin);
        const double height_limit = static_cast<double>(kCanvasHeight - 2 * kMargin);
        const double width_scale = width_limit / std::max(1.0, initial_bounds.width());
        const double height_scale = height_limit / std::max(1.0, initial_bounds.height());
        const double fit_scale = std::min({1.0, width_scale, height_scale}) * font_size_percent_ / 100.0;
        font.setPointSizeF(std::max(1.0, font.pointSizeF() * fit_scale));

        painter.setFont(font);
        painter.setPen(QColor::fromRgba(text_color_));
        painter.drawText(image.rect(), Qt::AlignCenter, display_text);
        painter.end();

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
    std::string font_face_{"Arial"};
    std::string font_style_{"Regular"};
    int font_size_points_{144};
    uint32_t font_flags_{0};
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
    obs_data_t *font = obs_data_create();
    obs_data_set_default_string(font, "face", "Arial");
    obs_data_set_default_string(font, "style", "Regular");
    obs_data_set_default_int(font, "size", 144);
    obs_data_set_default_int(font, "flags", 0);
    obs_data_set_default_obj(settings, "font", font);
    obs_data_release(font);

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
    obs_properties_add_font(properties, "font", obs_module_text("Settings.Font"));
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
