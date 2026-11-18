#include "countdown-source.hpp"

#include "countdownstudio/countdown.hpp"

#include <graphics/graphics.h>
#include <obs-data.h>
#include <obs-module.h>
#include <obs-properties.h>
#include <obs-source.h>

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QImage>
#include <QPainter>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <chrono>
#include <cmath>
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

std::string read_end_message(obs_data_t *settings)
{
    const char *message = obs_data_get_string(settings, "end_message");
    return message != nullptr ? message : obs_module_text("Settings.DefaultEndMessage");
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
          end_message_(read_end_message(settings)),
          start_animation_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "start_animation"), 0, 8))),
          change_animation_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "change_animation"), 0, 8))),
          animation_duration_ms_(static_cast<int>(std::clamp<int64_t>(
              obs_data_get_int(settings, "animation_duration_ms"), 100, 3000))),
          animation_easing_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "animation_easing"), 0, 2))),
          timer_(std::chrono::duration_cast<Countdown::Duration>(std::chrono::seconds{duration_seconds_})),
          pixels_(static_cast<std::size_t>(kCanvasWidth) * kCanvasHeight * kChannels, 0)
    {
        read_font_settings(settings);
        if (auto_start_) {
            timer_.start();
        }
        current_text_ = current_timer_text_locked();
        previous_text_ = current_text_;
        if (auto_start_) {
            trigger_start_animation_locked();
        }
        render_pixels_locked();
    }

    ~CountdownSource()
    {
        if (custom_font_id_ >= 0) {
            QFontDatabase::removeApplicationFont(custom_font_id_);
        }
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
        end_message_ = read_end_message(settings);
        const int new_start_animation = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "start_animation"), 0, 8));
        const int new_change_animation = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "change_animation"), 0, 8));
        const int new_animation_duration = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "animation_duration_ms"), 100, 3000));
        const int new_animation_easing = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "animation_easing"), 0, 2));
        const bool animation_settings_changed = new_start_animation != start_animation_
                                                || new_change_animation != change_animation_
                                                || new_animation_duration != animation_duration_ms_
                                                || new_animation_easing != animation_easing_;
        start_animation_ = new_start_animation;
        change_animation_ = new_change_animation;
        animation_duration_ms_ = new_animation_duration;
        animation_easing_ = new_animation_easing;
        if (animation_settings_changed) {
            animation_active_ = false;
            active_animation_preset_ = 0;
            previous_text_ = current_text_;
        }
        read_font_settings(settings);

        if (new_duration != duration_seconds_ || new_auto_start != auto_start_) {
            duration_seconds_ = new_duration;
            auto_start_ = new_auto_start;
            reset_timer_locked(auto_start_);
        } else {
            render_pixels_locked();
        }
    }

    void tick(float seconds)
    {
        std::lock_guard lock(mutex_);
        const bool was_animating = animation_active_;
        if (animation_active_) {
            animation_elapsed_seconds_ += std::max(0.0, static_cast<double>(seconds));
            const double duration_seconds = static_cast<double>(animation_duration_ms_) / 1000.0;
            if (animation_elapsed_seconds_ >= duration_seconds) {
                animation_active_ = false;
                active_animation_preset_ = 0;
                previous_text_ = current_text_;
            }
        }

        if (was_animating || display_seconds(timer_.remaining()) != last_display_seconds_) {
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
        refresh_display_text_locked();
        trigger_start_animation_locked();
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
        refresh_display_text_locked();
        if (start) {
            trigger_start_animation_locked();
        }
        render_pixels_locked();
    }

    std::string current_timer_text_locked() const
    {
        return timer_.state() == Countdown::State::finished
                   ? end_message_
                   : format_remaining(timer_.remaining(), time_format_);
    }

    void refresh_display_text_locked()
    {
        const std::string next_text = current_timer_text_locked();
        if (next_text == current_text_) {
            return;
        }

        previous_text_ = current_text_;
        current_text_ = next_text;
        active_animation_preset_ = change_animation_;
        animation_elapsed_seconds_ = 0.0;
        animation_active_ = active_animation_preset_ != 0 && animation_duration_ms_ > 0;
        if (!animation_active_) {
            active_animation_preset_ = 0;
            previous_text_ = current_text_;
        }
    }

    void trigger_start_animation_locked()
    {
        previous_text_ = current_text_;
        active_animation_preset_ = start_animation_;
        animation_elapsed_seconds_ = 0.0;
        animation_active_ = active_animation_preset_ != 0 && animation_duration_ms_ > 0;
    }

    void read_font_settings(obs_data_t *settings)
    {
        const char *custom_font_file = obs_data_get_string(settings, "custom_font_file");
        load_custom_font(custom_font_file != nullptr ? custom_font_file : "");

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

    void load_custom_font(const std::string &path)
    {
        if (path == custom_font_path_) {
            return;
        }

        custom_font_path_ = path;
        custom_font_family_.clear();
        if (custom_font_id_ >= 0) {
            QFontDatabase::removeApplicationFont(custom_font_id_);
            custom_font_id_ = -1;
        }
        if (custom_font_path_.empty()) {
            return;
        }

        const QString file_path = QString::fromUtf8(custom_font_path_.c_str());
        const int font_id = QFontDatabase::addApplicationFont(file_path);
        if (font_id < 0) {
            blog(LOG_WARNING, "[CountdownStudio] Could not load custom font file: %s", custom_font_path_.c_str());
            return;
        }

        const QStringList families = QFontDatabase::applicationFontFamilies(font_id);
        if (families.isEmpty()) {
            QFontDatabase::removeApplicationFont(font_id);
            blog(LOG_WARNING, "[CountdownStudio] No usable font family found in: %s", custom_font_path_.c_str());
            return;
        }

        custom_font_id_ = font_id;
        custom_font_family_ = families.front().toUtf8().toStdString();
    }

    void render_pixels_locked()
    {
        refresh_display_text_locked();

        QImage image(pixels_.data(), static_cast<int>(kCanvasWidth), static_cast<int>(kCanvasHeight),
                     static_cast<int>(kCanvasWidth * kChannels), QImage::Format_RGBA8888);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        if (show_background_) {
            painter.fillRect(image.rect(), QColor::fromRgba(background_color_));
        }

        const bool use_custom_font = !custom_font_family_.empty();
        const std::string &font_family = use_custom_font ? custom_font_family_ : font_face_;
        QFont font(QString::fromUtf8(font_family.c_str()));
        if (!use_custom_font) {
            font.setStyleName(QString::fromUtf8(font_style_.c_str()));
        }
        font.setBold((font_flags_ & OBS_FONT_BOLD) != 0);
        font.setItalic((font_flags_ & OBS_FONT_ITALIC) != 0);
        font.setUnderline((font_flags_ & OBS_FONT_UNDERLINE) != 0);
        font.setStrikeOut((font_flags_ & OBS_FONT_STRIKEOUT) != 0);
        font.setPointSize(font_size_points_);

        const QString display_text = QString::fromUtf8(current_text_.c_str());
        const QString previous_text = QString::fromUtf8(previous_text_.c_str());
        const QFontMetricsF initial_metrics(font);
        const QRectF current_bounds = initial_metrics.tightBoundingRect(display_text);
        const QRectF previous_bounds = initial_metrics.tightBoundingRect(previous_text);
        const double width_limit = static_cast<double>(kCanvasWidth - 2 * kMargin);
        const double height_limit = static_cast<double>(kCanvasHeight - 2 * kMargin);
        const double text_width = std::max(current_bounds.width(), previous_bounds.width());
        const double text_height = std::max(current_bounds.height(), previous_bounds.height());
        const double width_scale = width_limit / std::max(1.0, text_width);
        const double height_scale = height_limit / std::max(1.0, text_height);
        const double fit_scale = std::min({1.0, width_scale, height_scale}) * font_size_percent_ / 100.0;
        font.setPointSizeF(std::max(1.0, font.pointSizeF() * fit_scale));

        const double raw_progress = animation_active_
                                        ? std::clamp(animation_elapsed_seconds_ * 1000.0
                                                         / static_cast<double>(animation_duration_ms_),
                                                     0.0, 1.0)
                                        : 1.0;
        double progress = raw_progress;
        if (animation_easing_ == 1) {
            progress = 1.0 - std::pow(1.0 - raw_progress, 3.0);
        } else if (animation_easing_ == 2) {
            progress = raw_progress * raw_progress * (3.0 - 2.0 * raw_progress);
        }

        const double center_x = static_cast<double>(kCanvasWidth) / 2.0;
        const double center_y = static_cast<double>(kCanvasHeight) / 2.0;
        auto draw_layer = [&](const QString &text, double opacity, double scale, double offset_x, double offset_y,
                              bool reveal) {
            if (text.isEmpty() || opacity <= 0.0) {
                return;
            }

            painter.save();
            painter.setFont(font);
            painter.setPen(QColor::fromRgba(text_color_));
            painter.setOpacity(std::clamp(opacity, 0.0, 1.0));
            if (reveal) {
                const double line_width = QFontMetricsF(font).horizontalAdvance(text);
                const double left = std::max(0.0, (static_cast<double>(kCanvasWidth) - line_width) / 2.0);
                painter.setClipRect(QRectF(left, 0.0, line_width * progress, static_cast<double>(kCanvasHeight)),
                                    Qt::IntersectClip);
            }
            if (scale != 1.0 || offset_x != 0.0 || offset_y != 0.0) {
                painter.translate(center_x + offset_x, center_y + offset_y);
                painter.scale(scale, scale);
                painter.translate(-center_x, -center_y);
            }
            painter.drawText(image.rect(), Qt::AlignCenter, text);
            painter.restore();
        };

        if (!animation_active_) {
            draw_layer(display_text, 1.0, 1.0, 0.0, 0.0, false);
        } else {
            const double outgoing_opacity = 1.0 - progress;
            const double incoming_opacity = progress;
            constexpr double kPi = 3.14159265358979323846;
            switch (active_animation_preset_) {
            case 1: // Fade
                draw_layer(previous_text, outgoing_opacity, 1.0, 0.0, 0.0, false);
                draw_layer(display_text, incoming_opacity, 1.0, 0.0, 0.0, false);
                break;
            case 2: // Slide up
                draw_layer(previous_text, outgoing_opacity, 1.0, 0.0, -44.0 * progress, false);
                draw_layer(display_text, incoming_opacity, 1.0, 0.0, 44.0 * (1.0 - progress), false);
                break;
            case 3: // Slide left
                draw_layer(previous_text, outgoing_opacity, 1.0, -64.0 * progress, 0.0, false);
                draw_layer(display_text, incoming_opacity, 1.0, 64.0 * (1.0 - progress), 0.0, false);
                break;
            case 4: // Zoom
                draw_layer(previous_text, outgoing_opacity, 1.0 + 0.12 * progress, 0.0, 0.0, false);
                draw_layer(display_text, incoming_opacity, 0.78 + 0.22 * progress, 0.0, 0.0, false);
                break;
            case 5: { // Bounce
                const double bounce = 1.0 - std::exp(-7.0 * raw_progress) * std::cos(10.0 * raw_progress);
                const double scale = 0.78 + 0.22 * bounce;
                const double offset_y = 30.0 * (1.0 - bounce);
                draw_layer(previous_text, outgoing_opacity, 1.0, 0.0, -offset_y * 0.35, false);
                draw_layer(display_text, incoming_opacity, scale, 0.0, offset_y, false);
                break;
            }
            case 6: // Pulse
                draw_layer(previous_text, outgoing_opacity, 1.0, 0.0, 0.0, false);
                draw_layer(display_text, incoming_opacity,
                           1.0 + 0.12 * std::sin(raw_progress * kPi) * (1.0 - raw_progress), 0.0, 0.0, false);
                break;
            case 7: // Reveal
                draw_layer(previous_text, outgoing_opacity, 1.0, 0.0, 0.0, false);
                draw_layer(display_text, 1.0, 1.0, 0.0, 0.0, true);
                break;
            case 8: { // Shake
                const double shake = 15.0 * std::sin(raw_progress * 10.0 * kPi) * (1.0 - raw_progress);
                draw_layer(previous_text, outgoing_opacity, 1.0, -shake * 0.35, 0.0, false);
                draw_layer(display_text, incoming_opacity, 1.0, shake, 0.0, false);
                break;
            }
            default:
                draw_layer(display_text, 1.0, 1.0, 0.0, 0.0, false);
                break;
            }
        }

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
    std::string end_message_;
    int start_animation_;
    int change_animation_;
    int animation_duration_ms_;
    int animation_easing_;
    int active_animation_preset_{0};
    bool animation_active_{false};
    double animation_elapsed_seconds_{0.0};
    std::string current_text_;
    std::string previous_text_;
    std::string font_face_{"Arial"};
    std::string font_style_{"Regular"};
    std::string custom_font_path_;
    std::string custom_font_family_;
    int custom_font_id_{-1};
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

void source_tick(void *data, float seconds)
{
    static_cast<CountdownSource *>(data)->tick(seconds);
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

    obs_data_set_default_string(settings, "custom_font_file", "");
    obs_data_set_default_int(settings, "duration_seconds", kDefaultDurationSeconds);
    obs_data_set_default_bool(settings, "auto_start", true);
    obs_data_set_default_bool(settings, "show_background", false);
    obs_data_set_default_int(settings, "text_color", kDefaultTextColor);
    obs_data_set_default_int(settings, "background_color", kDefaultBackgroundColor);
    obs_data_set_default_int(settings, "font_size_percent", 100);
    obs_data_set_default_int(settings, "time_format", 0);
    obs_data_set_default_string(settings, "end_message", obs_module_text("Settings.DefaultEndMessage"));
    obs_data_set_default_int(settings, "start_animation", 1);
    obs_data_set_default_int(settings, "change_animation", 1);
    obs_data_set_default_int(settings, "animation_duration_ms", 350);
    obs_data_set_default_int(settings, "animation_easing", 1);
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
    obs_properties_add_path(properties, "custom_font_file", obs_module_text("Settings.CustomFontFile"),
                            OBS_PATH_FILE, "Font files (*.ttf *.otf *.ttc)", nullptr);
    obs_properties_add_int_slider(properties, "font_size_percent", obs_module_text("Settings.FontSize"), 25, 100, 1);
    auto *format = obs_properties_add_list(properties, "time_format", obs_module_text("Settings.TimeFormat"),
                                           OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(format, obs_module_text("Format.Automatic"), 0);
    obs_property_list_add_int(format, "MM:SS", 1);
    obs_property_list_add_int(format, "HH:MM:SS", 2);
    obs_properties_add_text(properties, "end_message", obs_module_text("Settings.EndMessage"), OBS_TEXT_DEFAULT);

    auto *animation = obs_properties_create();
    auto *start_animation = obs_properties_add_list(animation, "start_animation",
                                                    obs_module_text("Settings.StartAnimation"),
                                                    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    auto *change_animation = obs_properties_add_list(animation, "change_animation",
                                                     obs_module_text("Settings.ChangeAnimation"),
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    const char *animation_names[] = {
        "Animation.None", "Animation.Fade", "Animation.SlideUp", "Animation.SlideLeft",
        "Animation.Zoom", "Animation.Bounce", "Animation.Pulse", "Animation.Reveal", "Animation.Shake"};
    for (int preset = 0; preset < 9; ++preset) {
        const char *label = obs_module_text(animation_names[preset]);
        obs_property_list_add_int(start_animation, label, preset);
        obs_property_list_add_int(change_animation, label, preset);
    }
    obs_properties_add_int_slider(animation, "animation_duration_ms",
                                  obs_module_text("Settings.AnimationDuration"), 100, 3000, 50);
    auto *easing = obs_properties_add_list(animation, "animation_easing", obs_module_text("Settings.AnimationEasing"),
                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(easing, obs_module_text("Animation.EasingLinear"), 0);
    obs_property_list_add_int(easing, obs_module_text("Animation.EasingOut"), 1);
    obs_property_list_add_int(easing, obs_module_text("Animation.EasingInOut"), 2);
    obs_properties_add_group(properties, "animation_group", obs_module_text("Settings.AnimationGroup"),
                             OBS_GROUP_NORMAL, animation);

    if (data != nullptr) {
        auto *start = obs_properties_add_button2(properties, "start", obs_module_text("Controls.Start"),
                                                 start_clicked, data);
        auto *pause = obs_properties_add_button2(properties, "pause", obs_module_text("Controls.Pause"),
                                                 pause_clicked, data);
        auto *restart = obs_properties_add_button2(properties, "restart", obs_module_text("Controls.Restart"),
                                                   restart_clicked, data);
        auto *add_minute = obs_properties_add_button2(properties, "add_minute", obs_module_text("Controls.AddMinute"),
                                                      add_minute_clicked, data);
        auto *add_five_minutes = obs_properties_add_button2(properties, "add_five_minutes",
                                                            obs_module_text("Controls.AddFiveMinutes"),
                                                            add_five_minutes_clicked, data);
        obs_property_set_visible(start, false);
        obs_property_set_visible(pause, false);
        obs_property_set_visible(restart, false);
        obs_property_set_visible(add_minute, false);
        obs_property_set_visible(add_five_minutes, false);
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
