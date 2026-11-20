#include "countdown-source.hpp"

#include "countdownstudio/countdown.hpp"

#include <graphics/graphics.h>
#include <obs-data.h>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs-properties.h>
#include <obs-source.h>
#include <obs.h>

#include <QByteArray>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QImage>
#include <QMetaObject>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QString>
#include <QStringList>
#include <QTime>

#include <algorithm>
#include <array>
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
constexpr int kMomentCount = 3;
constexpr int kMomentShowMessage = 0;
constexpr int kMomentPause = 1;
constexpr int kMomentAddMinute = 2;
constexpr int kMomentSwitchScene = 3;
constexpr int kDisplayModeCountdown = 0;
constexpr int kDisplayModeClock = 1;
constexpr int kClockFrameRectangle = 0;
constexpr int kClockFrameRounded = 1;
constexpr int kClockFramePill = 2;
constexpr int kClockFrameEllipse = 3;

struct MomentRule {
    bool enabled{false};
    int64_t seconds{0};
    int action{kMomentShowMessage};
    std::string message;
    int message_duration_seconds{3};
    std::string scene;

    bool operator==(const MomentRule &) const = default;
};

std::string read_string_setting(obs_data_t *settings, const char *name)
{
    const char *value = obs_data_get_string(settings, name);
    return value != nullptr ? value : "";
}

std::array<MomentRule, kMomentCount> read_moment_rules(obs_data_t *settings)
{
    std::array<MomentRule, kMomentCount> rules{};
    for (int index = 0; index < kMomentCount; ++index) {
        const std::string prefix = "moment_" + std::to_string(index + 1);
        const std::string enabled_key = prefix + "_enabled";
        const std::string seconds_key = prefix + "_seconds";
        const std::string action_key = prefix + "_action";
        const std::string message_key = prefix + "_message";
        const std::string duration_key = prefix + "_message_duration_seconds";
        const std::string scene_key = prefix + "_scene";
        rules[index].enabled = obs_data_get_bool(settings, enabled_key.c_str());
        rules[index].seconds = std::clamp<int64_t>(obs_data_get_int(settings, seconds_key.c_str()), 1,
                                                   kMaximumDurationSeconds);
        rules[index].action = static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, action_key.c_str()),
                                                                   kMomentShowMessage, kMomentSwitchScene));
        rules[index].message = read_string_setting(settings, message_key.c_str());
        rules[index].message_duration_seconds = static_cast<int>(std::clamp<int64_t>(
            obs_data_get_int(settings, duration_key.c_str()), 1, 30));
        rules[index].scene = read_string_setting(settings, scene_key.c_str());
    }
    return rules;
}

void add_scene_list(obs_properties_t *properties, const char *name, const char *label)
{
    obs_property_t *list = obs_properties_add_list(properties, name, label, OBS_COMBO_TYPE_LIST,
                                                   OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(list, obs_module_text("Settings.NoScene"), "");
    struct obs_frontend_source_list scenes = {};
    obs_frontend_get_scenes(&scenes);
    for (size_t index = 0; index < scenes.sources.num; ++index) {
        obs_source_t *scene = scenes.sources.array[index];
        obs_property_list_add_string(list, obs_source_get_name(scene), obs_source_get_name(scene));
    }
    obs_frontend_source_list_free(&scenes);
}

void queue_scene_switch(const std::string &name)
{
    if (name.empty()) {
        return;
    }
    auto *main_window = static_cast<QObject *>(obs_frontend_get_main_window());
    if (main_window == nullptr) {
        return;
    }
    const QString scene_name = QString::fromUtf8(name.c_str());
    QMetaObject::invokeMethod(main_window, [scene_name]() {
        const QByteArray scene_utf8 = scene_name.toUtf8();
        obs_source_t *scene = obs_get_source_by_name(scene_utf8.constData());
        if (scene != nullptr) {
            obs_frontend_set_current_scene(scene);
            obs_source_release(scene);
        }
    }, Qt::QueuedConnection);
}

QColor qcolor_from_obs_rgba(uint32_t color)
{
    return QColor(static_cast<int>(color & 0xFF), static_cast<int>((color >> 8) & 0xFF),
                  static_cast<int>((color >> 16) & 0xFF), static_cast<int>((color >> 24) & 0xFF));
}

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

QString preview_custom_font_family(obs_data_t *settings, const char *path_key)
{
    const char *raw_path = obs_data_get_string(settings, path_key);
    const std::string path = raw_path != nullptr ? raw_path : "";
    static std::mutex preview_font_mutex;
    static std::string loaded_path;
    static QString loaded_family;
    static int loaded_font_id = -1;

    std::lock_guard lock(preview_font_mutex);
    if (path == loaded_path) {
        return loaded_family;
    }

    if (loaded_font_id >= 0) {
        QFontDatabase::removeApplicationFont(loaded_font_id);
        loaded_font_id = -1;
    }
    loaded_path = path;
    loaded_family.clear();
    if (path.empty()) {
        return loaded_family;
    }

    loaded_font_id = QFontDatabase::addApplicationFont(QString::fromUtf8(path.c_str()));
    if (loaded_font_id < 0) {
        return loaded_family;
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(loaded_font_id);
    if (!families.isEmpty()) {
        loaded_family = families.front();
    }
    return loaded_family;
}

QString live_preview_markup(obs_data_t *settings)
{
    const int display_mode = static_cast<int>(std::clamp<int64_t>(
        obs_data_get_int(settings, "display_mode"), kDisplayModeCountdown, kDisplayModeClock));
    const bool clock_mode = display_mode == kDisplayModeClock;
    QString sample;
    const char *preview_text = obs_data_get_string(settings, "preview_text");
    if (preview_text != nullptr && preview_text[0] != '\0') {
        sample = QString::fromUtf8(preview_text);
    } else if (clock_mode) {
        const int clock_format = static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "clock_format"), 0, 1));
        sample = QTime::currentTime().toString(clock_format == 0 ? QStringLiteral("HH:mm")
                                                                 : QStringLiteral("HH:mm:ss"));
    } else {
        const int time_format = static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "time_format"), 0, 2));
        const auto duration = std::chrono::seconds{read_duration(settings)};
        sample = QString::fromUtf8(format_remaining(duration, time_format).c_str());
    }

    const uint32_t text_color_value = static_cast<uint32_t>(
        obs_data_get_int(settings, clock_mode ? "clock_text_color" : "text_color"));
    QColor text_color = qcolor_from_obs_rgba(text_color_value);
    const bool show_background = clock_mode ? obs_data_get_bool(settings, "clock_frame_enabled")
                                            : obs_data_get_bool(settings, "show_background");
    const uint32_t background_color_value = static_cast<uint32_t>(
        obs_data_get_int(settings, clock_mode ? "clock_frame_color" : "background_color"));
    QColor background_color = qcolor_from_obs_rgba(background_color_value);
    const int size_percent = static_cast<int>(std::clamp<int64_t>(
        obs_data_get_int(settings, clock_mode ? "clock_font_size_percent" : "font_size_percent"), 25, 100));

    QString font_family = QStringLiteral("Arial");
    QString font_weight = QStringLiteral("400");
    QString font_style = QStringLiteral("normal");
    uint32_t font_flags = 0;
    obs_data_t *font = obs_data_get_obj(settings, clock_mode ? "clock_font" : "font");
    if (font != nullptr) {
        const char *face = obs_data_get_string(font, "face");
        if (face != nullptr && face[0] != '\0') {
            font_family = QString::fromUtf8(face);
        }
        font_flags = static_cast<uint32_t>(obs_data_get_int(font, "flags"));
        obs_data_release(font);
    }
    const QString custom_family = preview_custom_font_family(
        settings, clock_mode ? "clock_custom_font_file" : "custom_font_file");
    if (!custom_family.isEmpty()) {
        font_family = custom_family;
    }
    if ((font_flags & OBS_FONT_BOLD) != 0) {
        font_weight = QStringLiteral("700");
    }
    if ((font_flags & OBS_FONT_ITALIC) != 0) {
        font_style = QStringLiteral("italic");
    }
    font_family.replace(QLatin1Char('"'), QLatin1Char(' '));
    font_family.replace(QLatin1Char('\''), QLatin1Char(' '));

    auto rgba = [](const QColor &color) {
        return QStringLiteral("rgba(%1,%2,%3,%4)")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(color.alphaF(), 0, 'f', 3);
    };

    QString fill = show_background ? rgba(background_color) : QStringLiteral("transparent");
    int border_width = clock_mode && show_background
                           ? static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "clock_frame_border_width"),
                                                                  0, 20))
                           : 0;
    QColor border_color = qcolor_from_obs_rgba(static_cast<uint32_t>(
        obs_data_get_int(settings, "clock_frame_border_color")));
    const int frame_shape = static_cast<int>(std::clamp<int64_t>(
        obs_data_get_int(settings, "clock_frame_shape"), kClockFrameRectangle, kClockFrameEllipse));
    const int radius = static_cast<int>(std::clamp<int64_t>(
        obs_data_get_int(settings, "clock_frame_radius"), 0, 180));
    const int padding = clock_mode && show_background
                            ? static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "clock_frame_padding"),
                                                                   0, 120))
                            : 0;
    QString border_radius = QStringLiteral("0px");
    if (frame_shape == kClockFrameRounded) {
        border_radius = QStringLiteral("%1px").arg(radius);
    } else if (frame_shape == kClockFramePill) {
        border_radius = QStringLiteral("9999px");
    } else if (frame_shape == kClockFrameEllipse) {
        border_radius = QStringLiteral("50%");
    }

    const int font_size = 58 * size_percent / 100;
    const QString html = QStringLiteral(
        "<html><body style='margin:0;padding:0;background:transparent'>"
        "<table width='620' height='180' cellspacing='0' cellpadding='0'><tr><td align='center' valign='middle'>"
        "<table cellspacing='0' cellpadding='%1' style='background-color:%2;border:%3px solid %4;"
        "border-radius:%5;'><tr><td align='center'>"
        "<span style=\"font-family:'%6';font-size:%7px;color:%8;font-weight:%9;font-style:%10;\">%11</span>"
        "</td></tr></table></td></tr></table></body></html>");
    return html.arg(QString::number(padding))
        .arg(fill)
        .arg(QString::number(border_width))
        .arg(rgba(border_color))
        .arg(border_radius)
        .arg(font_family.toHtmlEscaped())
        .arg(QString::number(font_size))
        .arg(rgba(text_color))
        .arg(font_weight)
        .arg(font_style)
        .arg(sample.toHtmlEscaped());
}

class CountdownSource final {
public:
    explicit CountdownSource(obs_data_t *settings)
        : duration_seconds_(read_duration(settings)),
          display_mode_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "display_mode"),
                                                             kDisplayModeCountdown, kDisplayModeClock))),
          clock_format_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "clock_format"), 0, 1))),
          auto_start_(obs_data_get_bool(settings, "auto_start")),
          start_on_stream_start_(obs_data_get_bool(settings, "start_on_stream_start")),
          show_background_(obs_data_get_bool(settings, "show_background")),
          antialiasing_(obs_data_get_bool(settings, "antialiasing")),
          text_color_(static_cast<uint32_t>(obs_data_get_int(settings, "text_color"))),
          background_color_(static_cast<uint32_t>(obs_data_get_int(settings, "background_color"))),
          font_size_percent_(
              static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "font_size_percent"), 25, 100))),
          time_format_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "time_format"), 0, 2))),
          end_message_(read_end_message(settings)),
          switch_scene_on_end_(obs_data_get_bool(settings, "switch_scene_on_end")),
          end_scene_delay_seconds_(static_cast<int>(std::clamp<int64_t>(
              obs_data_get_int(settings, "end_scene_delay_seconds"), 0, 3600))),
          end_scene_name_(read_string_setting(settings, "end_scene_name")),
          moments_(read_moment_rules(settings)),
          start_animation_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "start_animation"), 0, 11))),
          change_animation_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "change_animation"), 0, 11))),
          animation_duration_ms_(static_cast<int>(std::clamp<int64_t>(
              obs_data_get_int(settings, "animation_duration_ms"), 100, 3000))),
          animation_easing_(static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "animation_easing"), 0, 2))),
          animate_digits_only_(obs_data_get_bool(settings, "animate_digits_only")),
          animation_stagger_ms_(static_cast<int>(std::clamp<int64_t>(
              obs_data_get_int(settings, "animation_stagger_ms"), 0, 250))),
          custom_animation_scale_start_percent_(static_cast<int>(std::clamp<int64_t>(
              obs_data_get_int(settings, "animation_custom_scale_start_percent"), 25, 200))),
          custom_animation_offset_x_(static_cast<int>(std::clamp<int64_t>(
              obs_data_get_int(settings, "animation_custom_offset_x"), -300, 300))),
          custom_animation_offset_y_(static_cast<int>(std::clamp<int64_t>(
              obs_data_get_int(settings, "animation_custom_offset_y"), -300, 300))),
          custom_animation_rotation_(static_cast<int>(std::clamp<int64_t>(
              obs_data_get_int(settings, "animation_custom_rotation"), -180, 180))),
          custom_animation_fade_(obs_data_get_bool(settings, "animation_custom_fade")),
          timer_(std::chrono::duration_cast<Countdown::Duration>(std::chrono::seconds{duration_seconds_})),
          pixels_(static_cast<std::size_t>(kCanvasWidth) * kCanvasHeight * kChannels, 0)
    {
        read_font_settings(settings);
        read_clock_appearance_settings(settings);
        read_clock_font_settings(settings);
        if (display_mode_ == kDisplayModeCountdown && auto_start_ && !start_on_stream_start_) {
            timer_.start();
        }
        current_text_ = current_timer_text_locked();
        previous_text_ = current_text_;
        if (display_mode_ == kDisplayModeCountdown && auto_start_ && !start_on_stream_start_) {
            trigger_start_animation_locked();
        }
        render_pixels_locked();
    }

    ~CountdownSource()
    {
        if (custom_font_id_ >= 0) {
            QFontDatabase::removeApplicationFont(custom_font_id_);
        }
        if (clock_custom_font_id_ >= 0) {
            QFontDatabase::removeApplicationFont(clock_custom_font_id_);
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
        const int new_display_mode = static_cast<int>(std::clamp<int64_t>(
            obs_data_get_int(settings, "display_mode"), kDisplayModeCountdown, kDisplayModeClock));
        const int new_clock_format = static_cast<int>(std::clamp<int64_t>(
            obs_data_get_int(settings, "clock_format"), 0, 1));
        const bool display_mode_changed = new_display_mode != display_mode_;
        const auto new_auto_start = obs_data_get_bool(settings, "auto_start");
        const auto new_start_on_stream_start = obs_data_get_bool(settings, "start_on_stream_start");
        show_background_ = obs_data_get_bool(settings, "show_background");
        antialiasing_ = obs_data_get_bool(settings, "antialiasing");
        text_color_ = static_cast<uint32_t>(obs_data_get_int(settings, "text_color"));
        background_color_ = static_cast<uint32_t>(obs_data_get_int(settings, "background_color"));
        font_size_percent_ = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "font_size_percent"), 25, 100));
        display_mode_ = new_display_mode;
        clock_format_ = new_clock_format;
        time_format_ = static_cast<int>(std::clamp<int64_t>(obs_data_get_int(settings, "time_format"), 0, 2));
        end_message_ = read_end_message(settings);
        const bool new_switch_scene_on_end = obs_data_get_bool(settings, "switch_scene_on_end");
        const int new_end_scene_delay = static_cast<int>(std::clamp<int64_t>(
            obs_data_get_int(settings, "end_scene_delay_seconds"), 0, 3600));
        const std::string new_end_scene_name = read_string_setting(settings, "end_scene_name");
        if (new_switch_scene_on_end != switch_scene_on_end_ || new_end_scene_delay != end_scene_delay_seconds_
            || new_end_scene_name != end_scene_name_) {
            end_scene_elapsed_seconds_ = 0.0;
            end_scene_fired_ = false;
        }
        switch_scene_on_end_ = new_switch_scene_on_end;
        end_scene_delay_seconds_ = new_end_scene_delay;
        end_scene_name_ = new_end_scene_name;
        const auto new_moments = read_moment_rules(settings);
        if (new_moments != moments_) {
            moments_ = new_moments;
            moment_fired_.fill(false);
        }
        const int new_start_animation = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "start_animation"), 0, 11));
        const int new_change_animation = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "change_animation"), 0, 11));
        const int new_animation_duration = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "animation_duration_ms"), 100, 3000));
        const int new_animation_easing = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "animation_easing"), 0, 2));
        const bool new_animate_digits_only = obs_data_get_bool(settings, "animate_digits_only");
        const int new_animation_stagger = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "animation_stagger_ms"), 0, 250));
        const int new_custom_scale = static_cast<int>(std::clamp<int64_t>(
            obs_data_get_int(settings, "animation_custom_scale_start_percent"), 25, 200));
        const int new_custom_offset_x = static_cast<int>(std::clamp<int64_t>(
            obs_data_get_int(settings, "animation_custom_offset_x"), -300, 300));
        const int new_custom_offset_y = static_cast<int>(std::clamp<int64_t>(
            obs_data_get_int(settings, "animation_custom_offset_y"), -300, 300));
        const int new_custom_rotation = static_cast<int>(std::clamp<int64_t>(
            obs_data_get_int(settings, "animation_custom_rotation"), -180, 180));
        const bool new_custom_fade = obs_data_get_bool(settings, "animation_custom_fade");
        const bool animation_settings_changed = new_custom_scale != custom_animation_scale_start_percent_
                                                || new_custom_offset_x != custom_animation_offset_x_
                                                || new_custom_offset_y != custom_animation_offset_y_
                                                || new_custom_rotation != custom_animation_rotation_
                                                || new_custom_fade != custom_animation_fade_
                                                || new_start_animation != start_animation_
                                                || new_change_animation != change_animation_
                                                || new_animation_duration != animation_duration_ms_
                                                || new_animation_easing != animation_easing_
                                                || new_animate_digits_only != animate_digits_only_
                                                || new_animation_stagger != animation_stagger_ms_;
        start_animation_ = new_start_animation;
        change_animation_ = new_change_animation;
        animation_duration_ms_ = new_animation_duration;
        animation_easing_ = new_animation_easing;
        animate_digits_only_ = new_animate_digits_only;
        animation_stagger_ms_ = new_animation_stagger;
        custom_animation_scale_start_percent_ = new_custom_scale;
        custom_animation_offset_x_ = new_custom_offset_x;
        custom_animation_offset_y_ = new_custom_offset_y;
        custom_animation_rotation_ = new_custom_rotation;
        custom_animation_fade_ = new_custom_fade;
        if (animation_settings_changed) {
            animation_active_ = false;
            active_animation_preset_ = 0;
            active_animation_is_change_ = false;
            previous_text_ = current_text_;
        }
        read_font_settings(settings);
        read_clock_appearance_settings(settings);
        read_clock_font_settings(settings);

        if (new_duration != duration_seconds_ || new_auto_start != auto_start_
            || new_start_on_stream_start != start_on_stream_start_ || display_mode_changed) {
            duration_seconds_ = new_duration;
            auto_start_ = new_auto_start;
            start_on_stream_start_ = new_start_on_stream_start;
            reset_timer_locked(display_mode_ == kDisplayModeCountdown && auto_start_ && !start_on_stream_start_);
        } else {
            render_pixels_locked();
        }
    }

    void tick(float seconds)
    {
        const double delta_seconds = std::max(0.0, static_cast<double>(seconds));
        std::vector<std::string> scenes_to_switch;
        {
            std::lock_guard lock(mutex_);
            const bool was_animating = animation_active_;
            bool should_render = false;
            if (moment_overlay_remaining_seconds_ > 0.0) {
                moment_overlay_remaining_seconds_ = std::max(0.0, moment_overlay_remaining_seconds_ - delta_seconds);
                if (moment_overlay_remaining_seconds_ == 0.0) {
                    moment_overlay_text_.clear();
                    refresh_display_text_locked();
                    should_render = true;
                }
            }

            if (display_mode_ == kDisplayModeClock) {
                refresh_display_text_locked();
                should_render = should_render || current_text_ != last_rendered_text_;
            }

            if (display_mode_ == kDisplayModeCountdown && timer_.state() == Countdown::State::running) {
                const int64_t remaining_seconds = display_seconds(timer_.remaining());
                for (int index = 0; index < kMomentCount; ++index) {
                    const MomentRule &rule = moments_[index];
                    if (!rule.enabled || moment_fired_[index] || remaining_seconds > rule.seconds) {
                        continue;
                    }
                    moment_fired_[index] = true;
                    switch (rule.action) {
                    case kMomentShowMessage:
                        moment_overlay_text_ = rule.message.empty()
                                                   ? obs_module_text("Settings.DefaultMomentMessage")
                                                   : rule.message;
                        moment_overlay_remaining_seconds_ = rule.message_duration_seconds;
                        refresh_display_text_locked();
                        should_render = true;
                        break;
                    case kMomentPause:
                        timer_.pause();
                        should_render = true;
                        break;
                    case kMomentAddMinute:
                        timer_.add_time(std::chrono::minutes{1});
                        should_render = true;
                        break;
                    case kMomentSwitchScene:
                        if (!rule.scene.empty()) {
                            scenes_to_switch.push_back(rule.scene);
                        }
                        break;
                    }
                    if (rule.action == kMomentPause) {
                        break;
                    }
                }
            }

            if (display_mode_ == kDisplayModeCountdown && timer_.state() == Countdown::State::finished
                && switch_scene_on_end_ && !end_scene_fired_) {
                end_scene_elapsed_seconds_ += delta_seconds;
                if (end_scene_elapsed_seconds_ >= static_cast<double>(end_scene_delay_seconds_)) {
                    end_scene_fired_ = true;
                    if (!end_scene_name_.empty()) {
                        scenes_to_switch.push_back(end_scene_name_);
                    }
                }
            }

            if (animation_active_) {
                animation_elapsed_seconds_ += delta_seconds;
                const double duration_seconds = static_cast<double>(animation_duration_ms_) / 1000.0;
                if (animation_elapsed_seconds_ >= duration_seconds) {
                    animation_active_ = false;
                    active_animation_preset_ = 0;
                    active_animation_is_change_ = false;
                    previous_text_ = current_text_;
                }
            }

            const bool text_changed = display_mode_ == kDisplayModeClock
                                          ? current_text_ != last_rendered_text_
                                          : display_seconds(timer_.remaining()) != last_display_seconds_;
            if (should_render || was_animating || text_changed) {
                render_pixels_locked();
            }
        }
        for (const std::string &scene : scenes_to_switch) {
            queue_scene_switch(scene);
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
        if (display_mode_ == kDisplayModeClock) {
            return;
        }
        timer_.start();
        refresh_display_text_locked();
        trigger_start_animation_locked();
        render_pixels_locked();
    }

    void pause()
    {
        std::lock_guard lock(mutex_);
        if (display_mode_ == kDisplayModeClock) {
            return;
        }
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
        if (display_mode_ == kDisplayModeClock) {
            return;
        }
        reset_timer_locked(true);
    }

    int display_mode()
    {
        std::lock_guard lock(mutex_);
        return display_mode_;
    }

    obs_data_t *preview_settings()
    {
        std::lock_guard lock(mutex_);
        obs_data_t *settings = obs_data_create();
        obs_data_set_int(settings, "display_mode", display_mode_);
        obs_data_set_int(settings, "clock_format", clock_format_);
        obs_data_set_int(settings, "time_format", time_format_);
        obs_data_set_int(settings, "duration_seconds", duration_seconds_);
        obs_data_set_string(settings, "preview_text", current_timer_text_locked().c_str());
        obs_data_set_bool(settings, "show_background", show_background_);
        obs_data_set_bool(settings, "antialiasing", antialiasing_);
        obs_data_set_int(settings, "text_color", text_color_);
        obs_data_set_int(settings, "background_color", background_color_);
        obs_data_set_int(settings, "font_size_percent", font_size_percent_);
        obs_data_set_string(settings, "custom_font_file", custom_font_path_.c_str());
        obs_data_set_int(settings, "clock_text_color", clock_text_color_);
        obs_data_set_int(settings, "clock_font_size_percent", clock_font_size_percent_);
        obs_data_set_string(settings, "clock_custom_font_file", clock_custom_font_path_.c_str());
        obs_data_set_bool(settings, "clock_frame_enabled", clock_frame_enabled_);
        obs_data_set_int(settings, "clock_frame_color", clock_frame_color_);
        obs_data_set_int(settings, "clock_frame_shape", clock_frame_shape_);
        obs_data_set_int(settings, "clock_frame_radius", clock_frame_radius_);
        obs_data_set_int(settings, "clock_frame_padding", clock_frame_padding_);
        obs_data_set_int(settings, "clock_frame_border_color", clock_frame_border_color_);
        obs_data_set_int(settings, "clock_frame_border_width", clock_frame_border_width_);

        auto add_font = [settings](const char *key, const std::string &face, const std::string &style,
                                   int size, uint32_t flags) {
            obs_data_t *font = obs_data_create();
            obs_data_set_string(font, "face", face.c_str());
            obs_data_set_string(font, "style", style.c_str());
            obs_data_set_int(font, "size", size);
            obs_data_set_int(font, "flags", flags);
            obs_data_set_obj(settings, key, font);
            obs_data_release(font);
        };
        add_font("font", font_face_, font_style_, font_size_points_, font_flags_);
        add_font("clock_font", clock_font_face_, clock_font_style_, clock_font_size_points_, clock_font_flags_);
        return settings;
    }

private:
    void add_time(Countdown::Duration extra)
    {
        std::lock_guard lock(mutex_);
        if (display_mode_ == kDisplayModeClock) {
            return;
        }
        const bool was_finished = timer_.state() == Countdown::State::finished;
        timer_.add_time(extra);
        if (was_finished) {
            moment_fired_.fill(false);
            end_scene_elapsed_seconds_ = 0.0;
            end_scene_fired_ = false;
        }
        render_pixels_locked();
    }

    void reset_timer_locked(bool start)
    {
        moment_fired_.fill(false);
        moment_overlay_text_.clear();
        moment_overlay_remaining_seconds_ = 0.0;
        end_scene_elapsed_seconds_ = 0.0;
        end_scene_fired_ = false;
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
        if (display_mode_ == kDisplayModeClock) {
            const QString format = clock_format_ == 0 ? QStringLiteral("HH:mm") : QStringLiteral("HH:mm:ss");
            return QTime::currentTime().toString(format).toUtf8().toStdString();
        }
        if (moment_overlay_remaining_seconds_ > 0.0 && !moment_overlay_text_.empty()) {
            return moment_overlay_text_;
        }
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
        active_animation_is_change_ = true;
        active_animation_preset_ = change_animation_;
        animation_elapsed_seconds_ = 0.0;
        animation_active_ = active_animation_preset_ != 0 && animation_duration_ms_ > 0;
        if (!animation_active_) {
            active_animation_preset_ = 0;
            active_animation_is_change_ = false;
            previous_text_ = current_text_;
        }
    }

    void trigger_start_animation_locked()
    {
        previous_text_ = current_text_;
        active_animation_is_change_ = false;
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

    void read_clock_appearance_settings(obs_data_t *settings)
    {
        clock_text_color_ = static_cast<uint32_t>(obs_data_get_int(settings, "clock_text_color"));
        clock_font_size_percent_ = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "clock_font_size_percent"), 25, 100));
        clock_frame_enabled_ = obs_data_get_bool(settings, "clock_frame_enabled");
        clock_frame_color_ = static_cast<uint32_t>(obs_data_get_int(settings, "clock_frame_color"));
        clock_frame_shape_ = static_cast<int>(std::clamp<int64_t>(
            obs_data_get_int(settings, "clock_frame_shape"), kClockFrameRectangle, kClockFrameEllipse));
        clock_frame_radius_ = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "clock_frame_radius"), 0, 180));
        clock_frame_padding_ = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "clock_frame_padding"), 0, 120));
        clock_frame_border_color_ = static_cast<uint32_t>(obs_data_get_int(settings, "clock_frame_border_color"));
        clock_frame_border_width_ = static_cast<int>(
            std::clamp<int64_t>(obs_data_get_int(settings, "clock_frame_border_width"), 0, 20));
    }

    void read_clock_font_settings(obs_data_t *settings)
    {
        const char *custom_font_file = obs_data_get_string(settings, "clock_custom_font_file");
        load_clock_custom_font(custom_font_file != nullptr ? custom_font_file : "");

        obs_data_t *font = obs_data_get_obj(settings, "clock_font");
        if (font == nullptr) {
            return;
        }

        const char *face = obs_data_get_string(font, "face");
        const char *style = obs_data_get_string(font, "style");
        if (face != nullptr && face[0] != ' ') {
            clock_font_face_ = face;
        }
        if (style != nullptr) {
            clock_font_style_ = style;
        }
        clock_font_size_points_ = static_cast<int>(std::clamp<int64_t>(obs_data_get_int(font, "size"), 1, 1000));
        clock_font_flags_ = static_cast<uint32_t>(obs_data_get_int(font, "flags"));
        obs_data_release(font);
    }

    void load_clock_custom_font(const std::string &path)
    {
        if (path == clock_custom_font_path_) {
            return;
        }

        clock_custom_font_path_ = path;
        clock_custom_font_family_.clear();
        if (clock_custom_font_id_ >= 0) {
            QFontDatabase::removeApplicationFont(clock_custom_font_id_);
            clock_custom_font_id_ = -1;
        }
        if (clock_custom_font_path_.empty()) {
            return;
        }

        const int font_id = QFontDatabase::addApplicationFont(QString::fromUtf8(clock_custom_font_path_.c_str()));
        if (font_id < 0) {
            blog(LOG_WARNING, "[CountdownStudio] Could not load custom clock font file: %s",
                 clock_custom_font_path_.c_str());
            return;
        }

        const QStringList families = QFontDatabase::applicationFontFamilies(font_id);
        if (families.isEmpty()) {
            QFontDatabase::removeApplicationFont(font_id);
            blog(LOG_WARNING, "[CountdownStudio] No usable custom clock font family found: %s",
                 clock_custom_font_path_.c_str());
            return;
        }

        clock_custom_font_id_ = font_id;
        clock_custom_font_family_ = families.front().toUtf8().toStdString();
    }

    void render_pixels_locked()
    {
        refresh_display_text_locked();

        QImage image(pixels_.data(), static_cast<int>(kCanvasWidth), static_cast<int>(kCanvasHeight),
                     static_cast<int>(kCanvasWidth * kChannels), QImage::Format_RGBA8888);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, antialiasing_);
        painter.setRenderHint(QPainter::TextAntialiasing, antialiasing_);
        const bool clock_mode = display_mode_ == kDisplayModeClock;
        if (!clock_mode && show_background_) {
            painter.fillRect(image.rect(), qcolor_from_obs_rgba(background_color_));
        }

        const std::string &active_custom_font_family = clock_mode ? clock_custom_font_family_ : custom_font_family_;
        const bool use_custom_font = !active_custom_font_family.empty();
        const std::string &font_family = use_custom_font ? active_custom_font_family
                                                         : (clock_mode ? clock_font_face_ : font_face_);
        const std::string &font_style = clock_mode ? clock_font_style_ : font_style_;
        const uint32_t font_flags = clock_mode ? clock_font_flags_ : font_flags_;
        const int font_size_points = clock_mode ? clock_font_size_points_ : font_size_points_;
        const int active_font_size_percent = clock_mode ? clock_font_size_percent_ : font_size_percent_;
        const uint32_t active_text_color = clock_mode ? clock_text_color_ : text_color_;
        QFont font(QString::fromUtf8(font_family.c_str()));
        if (!use_custom_font) {
            font.setStyleName(QString::fromUtf8(font_style.c_str()));
        }
        font.setBold((font_flags & OBS_FONT_BOLD) != 0);
        font.setItalic((font_flags & OBS_FONT_ITALIC) != 0);
        font.setUnderline((font_flags & OBS_FONT_UNDERLINE) != 0);
        font.setStrikeOut((font_flags & OBS_FONT_STRIKEOUT) != 0);
        font.setPointSize(font_size_points);

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
        const double fit_scale = std::min({1.0, width_scale, height_scale}) * active_font_size_percent / 100.0;
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
        if (clock_mode && clock_frame_enabled_) {
            const QFontMetricsF frame_metrics(font);
            const double text_width = frame_metrics.horizontalAdvance(display_text);
            const double text_height = frame_metrics.height();
            const double padding = static_cast<double>(clock_frame_padding_);
            QRectF frame_rect(center_x - text_width / 2.0 - padding,
                              center_y - text_height / 2.0 - padding,
                              text_width + 2.0 * padding,
                              text_height + 2.0 * padding);
            const double border_width = static_cast<double>(clock_frame_border_width_);
            frame_rect.adjust(border_width / 2.0, border_width / 2.0, -border_width / 2.0, -border_width / 2.0);
            painter.setBrush(qcolor_from_obs_rgba(clock_frame_color_));
            painter.setPen(clock_frame_border_width_ > 0
                               ? QPen(qcolor_from_obs_rgba(clock_frame_border_color_), border_width)
                               : QPen(Qt::NoPen));
            switch (clock_frame_shape_) {
            case kClockFrameRounded:
                painter.drawRoundedRect(frame_rect, static_cast<double>(clock_frame_radius_),
                                         static_cast<double>(clock_frame_radius_), Qt::AbsoluteSize);
                break;
            case kClockFramePill:
                painter.drawRoundedRect(frame_rect, frame_rect.height() / 2.0, frame_rect.height() / 2.0,
                                         Qt::AbsoluteSize);
                break;
            case kClockFrameEllipse:
                painter.drawEllipse(frame_rect);
                break;
            default:
                painter.drawRect(frame_rect);
                break;
            }
        }
        auto draw_layer = [&](const QString &text, double opacity, double scale, double offset_x, double offset_y,
                              bool reveal, double rotation_degrees = 0.0) {
            if (text.isEmpty() || opacity <= 0.0) {
                return;
            }

            painter.save();
            painter.setFont(font);
            painter.setPen(qcolor_from_obs_rgba(active_text_color));
            painter.setOpacity(std::clamp(opacity, 0.0, 1.0));
            if (reveal) {
                const double line_width = QFontMetricsF(font).horizontalAdvance(text);
                const double left = std::max(0.0, (static_cast<double>(kCanvasWidth) - line_width) / 2.0);
                painter.setClipRect(QRectF(left, 0.0, line_width * progress, static_cast<double>(kCanvasHeight)),
                                    Qt::IntersectClip);
            }
            if (scale != 1.0 || offset_x != 0.0 || offset_y != 0.0 || rotation_degrees != 0.0) {
                painter.translate(center_x + offset_x, center_y + offset_y);
                painter.rotate(rotation_degrees);
                painter.scale(scale, scale);
                painter.translate(-center_x, -center_y);
            }
            painter.drawText(image.rect(), Qt::AlignCenter, text);
            painter.restore();
        };

        auto ease_progress = [&](double value) {
            if (animation_easing_ == 1) {
                return 1.0 - std::pow(1.0 - value, 3.0);
            }
            if (animation_easing_ == 2) {
                return value * value * (3.0 - 2.0 * value);
            }
            return value;
        };

        auto draw_digit_transition = [&]() {
            if (!animation_active_ || !active_animation_is_change_ || !animate_digits_only_
                || timer_.state() == Countdown::State::finished
                || display_text.size() != previous_text.size()) {
                return false;
            }

            for (int index = 0; index < display_text.size(); ++index) {
                const QChar previous = previous_text[index];
                const QChar current = display_text[index];
                if ((previous == QLatin1Char(':')) != (current == QLatin1Char(':'))) {
                    return false;
                }
                if (previous != QLatin1Char(':') && (!previous.isDigit() || !current.isDigit())) {
                    return false;
                }
            }

            QFontMetricsF metrics(font);
            double digit_cell_width = 0.0;
            for (int digit = 0; digit <= 9; ++digit) {
                digit_cell_width = std::max(digit_cell_width,
                                            metrics.horizontalAdvance(QString::number(digit)));
            }
            const double separator_width = metrics.horizontalAdvance(QStringLiteral(":"));
            double total_width = 0.0;
            for (int index = 0; index < display_text.size(); ++index) {
                total_width += display_text[index] == QLatin1Char(':') ? separator_width : digit_cell_width;
            }
            double cell_x = center_x - total_width / 2.0;
            const double line_top = (static_cast<double>(kCanvasHeight) - metrics.height()) / 2.0;
            const double baseline = line_top + metrics.ascent();
            const double cell_height = metrics.height();
            constexpr double kPi = 3.14159265358979323846;
            std::vector<int> changed_digits_from_right;
            for (int index = display_text.size() - 1; index >= 0; --index) {
                if (previous_text[index] != display_text[index]) {
                    changed_digits_from_right.push_back(index);
                }
            }
            const int changed_count = static_cast<int>(changed_digits_from_right.size());
            const double total_stagger = std::min(
                0.8, static_cast<double>(animation_stagger_ms_) * std::max(0, changed_count - 1)
                         / static_cast<double>(animation_duration_ms_));

            auto draw_glyph = [&](QChar character, double x, double width, double opacity,
                                  double scale_x, double scale_y, double offset_x, double offset_y,
                                  double reveal_fraction, double rotation_degrees = 0.0) {
                if (opacity <= 0.0 || reveal_fraction <= 0.0) {
                    return;
                }
                const QString glyph(character);
                const double glyph_width = metrics.horizontalAdvance(glyph);
                const double glyph_x = x + (width - glyph_width) / 2.0;
                const double glyph_center_x = x + width / 2.0;
                painter.save();
                painter.setFont(font);
                painter.setPen(qcolor_from_obs_rgba(active_text_color));
                painter.setOpacity(std::clamp(opacity, 0.0, 1.0));
                painter.setClipRect(QRectF(x, 0.0, width * std::clamp(reveal_fraction, 0.0, 1.0),
                                           static_cast<double>(kCanvasHeight)), Qt::IntersectClip);
                painter.translate(glyph_center_x + offset_x, center_y + offset_y);
                painter.rotate(rotation_degrees);
                painter.scale(scale_x, scale_y);
                painter.translate(-glyph_center_x, -center_y);
                painter.drawText(QPointF(glyph_x, baseline), glyph);
                painter.restore();
            };

            for (int index = 0; index < display_text.size(); ++index) {
                const QChar previous = previous_text[index];
                const QChar current = display_text[index];
                const bool is_separator = current == QLatin1Char(':');
                const double cell_width = is_separator ? separator_width : digit_cell_width;
                if (previous == current || is_separator) {
                    draw_glyph(current, cell_x, cell_width, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0);
                } else {
                    const auto rank = std::find(changed_digits_from_right.begin(),
                                                changed_digits_from_right.end(), index)
                                      - changed_digits_from_right.begin();
                    const double start_delay = changed_count > 1
                                                   ? total_stagger * static_cast<double>(rank)
                                                         / static_cast<double>(changed_count - 1)
                                                   : 0.0;
                    const double local_raw = std::clamp(
                        (raw_progress - start_delay) / std::max(0.001, 1.0 - start_delay), 0.0, 1.0);
                    const double digit_progress = ease_progress(local_raw);
                    const double outgoing_opacity = 1.0 - digit_progress;
                    const double incoming_opacity = digit_progress;
                    switch (active_animation_preset_) {
                    case 1: // Fade
                        draw_glyph(previous, cell_x, cell_width, outgoing_opacity, 1.0, 1.0, 0.0, 0.0, 1.0);
                        draw_glyph(current, cell_x, cell_width, incoming_opacity, 1.0, 1.0, 0.0, 0.0, 1.0);
                        break;
                    case 2: // Slide up
                        draw_glyph(previous, cell_x, cell_width, 1.0, 1.0, 1.0, 0.0,
                                   -cell_height * digit_progress, 1.0);
                        draw_glyph(current, cell_x, cell_width, 1.0, 1.0, 1.0, 0.0,
                                   cell_height * (1.0 - digit_progress), 1.0);
                        break;
                    case 3: // Slide left
                        draw_glyph(previous, cell_x, cell_width, 1.0, 1.0, 1.0,
                                   -cell_width * digit_progress, 0.0, 1.0);
                        draw_glyph(current, cell_x, cell_width, 1.0, 1.0, 1.0,
                                   cell_width * (1.0 - digit_progress), 0.0, 1.0);
                        break;
                    case 4: // Zoom
                        draw_glyph(previous, cell_x, cell_width, outgoing_opacity,
                                   1.0 + 0.12 * digit_progress, 1.0 + 0.12 * digit_progress, 0.0, 0.0, 1.0);
                        draw_glyph(current, cell_x, cell_width, incoming_opacity,
                                   0.75 + 0.25 * digit_progress, 0.75 + 0.25 * digit_progress, 0.0, 0.0, 1.0);
                        break;
                    case 5: { // Bounce
                        const double bounce = 1.0 - std::exp(-7.0 * local_raw) * std::cos(10.0 * local_raw);
                        const double scale = 0.78 + 0.22 * bounce;
                        const double offset_y = cell_height * 0.22 * (1.0 - bounce);
                        draw_glyph(previous, cell_x, cell_width, outgoing_opacity,
                                   1.0, 1.0, 0.0, -offset_y * 0.35, 1.0);
                        draw_glyph(current, cell_x, cell_width, incoming_opacity,
                                   scale, scale, 0.0, offset_y, 1.0);
                        break;
                    }
                    case 6: // Pulse
                        draw_glyph(previous, cell_x, cell_width, outgoing_opacity, 1.0, 1.0, 0.0, 0.0, 1.0);
                        draw_glyph(current, cell_x, cell_width, incoming_opacity,
                                   1.0 + 0.16 * std::sin(local_raw * kPi) * (1.0 - local_raw),
                                   1.0 + 0.16 * std::sin(local_raw * kPi) * (1.0 - local_raw), 0.0, 0.0, 1.0);
                        break;
                    case 7: // Reveal
                        draw_glyph(previous, cell_x, cell_width, outgoing_opacity, 1.0, 1.0, 0.0, 0.0, 1.0);
                        draw_glyph(current, cell_x, cell_width, 1.0, 1.0, 1.0, 0.0, 0.0, digit_progress);
                        break;
                    case 8: { // Shake
                        const double shake = cell_width * 0.18 * std::sin(local_raw * 10.0 * kPi)
                                             * (1.0 - local_raw);
                        draw_glyph(previous, cell_x, cell_width, outgoing_opacity,
                                   1.0, 1.0, -shake * 0.35, 0.0, 1.0);
                        draw_glyph(current, cell_x, cell_width, incoming_opacity, 1.0, 1.0, shake, 0.0, 1.0);
                        break;
                    }
                    case 9: // Flip
                        if (local_raw < 0.5) {
                            draw_glyph(previous, cell_x, cell_width, 1.0, 1.0,
                                       std::max(0.01, 1.0 - 2.0 * local_raw), 0.0, 0.0, 1.0);
                        } else {
                            draw_glyph(current, cell_x, cell_width, 1.0, 1.0,
                                       std::max(0.01, 2.0 * local_raw - 1.0), 0.0, 0.0, 1.0);
                        }
                        break;
                    case 10: { // Roll
                        const double direction = current.digitValue() < previous.digitValue() ? -1.0 : 1.0;
                        draw_glyph(previous, cell_x, cell_width, 1.0, 1.0, 1.0, 0.0,
                                   direction * cell_height * digit_progress, 1.0);
                        draw_glyph(current, cell_x, cell_width, 1.0, 1.0, 1.0, 0.0,
                                   -direction * cell_height * (1.0 - digit_progress), 1.0);
                        break;
                    }
                    case 11: { // Custom transform
                        const double scale = static_cast<double>(custom_animation_scale_start_percent_) / 100.0
                                             + (1.0 - static_cast<double>(custom_animation_scale_start_percent_) / 100.0)
                                                   * digit_progress;
                        const double offset_x = static_cast<double>(custom_animation_offset_x_)
                                                * (1.0 - digit_progress);
                        const double offset_y = static_cast<double>(custom_animation_offset_y_)
                                                * (1.0 - digit_progress);
                        const double rotation = static_cast<double>(custom_animation_rotation_)
                                                * (1.0 - digit_progress);
                        const double old_opacity = custom_animation_fade_ ? outgoing_opacity : 1.0;
                        const double new_opacity = custom_animation_fade_ ? incoming_opacity : 1.0;
                        draw_glyph(previous, cell_x, cell_width, old_opacity, 1.0, 1.0, -offset_x * 0.35,
                                   -offset_y * 0.35, 1.0, -rotation * 0.35);
                        draw_glyph(current, cell_x, cell_width, new_opacity, scale, scale,
                                   offset_x, offset_y, 1.0, rotation);
                        break;
                    }
                    default:
                        draw_glyph(previous, cell_x, cell_width, outgoing_opacity, 1.0, 1.0, 0.0, 0.0, 1.0);
                        draw_glyph(current, cell_x, cell_width, incoming_opacity, 1.0, 1.0, 0.0, 0.0, 1.0);
                        break;
                    }
                }
                cell_x += cell_width;
            }
            return changed_count > 0;
        };

        auto draw_full_flip = [&](const QString &text, double scale_y) {
            if (text.isEmpty()) {
                return;
            }
            painter.save();
            painter.setFont(font);
            painter.setPen(qcolor_from_obs_rgba(active_text_color));
            painter.translate(center_x, center_y);
            painter.scale(1.0, scale_y);
            painter.translate(-center_x, -center_y);
            painter.drawText(image.rect(), Qt::AlignCenter, text);
            painter.restore();
        };

        if (!animation_active_) {
            draw_layer(display_text, 1.0, 1.0, 0.0, 0.0, false);
        } else if (draw_digit_transition()) {
            // Unchanged digits and separators remain fixed while changed digits animate.
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
            case 5: {
                const double bounce = 1.0 - std::exp(-7.0 * raw_progress) * std::cos(10.0 * raw_progress);
                const double scale = 0.78 + 0.22 * bounce;
                const double offset_y = 30.0 * (1.0 - bounce);
                draw_layer(previous_text, outgoing_opacity, 1.0, 0.0, -offset_y * 0.35, false);
                draw_layer(display_text, incoming_opacity, scale, 0.0, offset_y, false);
                break;
            }
            case 6:
                draw_layer(previous_text, outgoing_opacity, 1.0, 0.0, 0.0, false);
                draw_layer(display_text, incoming_opacity,
                           1.0 + 0.12 * std::sin(raw_progress * kPi) * (1.0 - raw_progress), 0.0, 0.0, false);
                break;
            case 7:
                draw_layer(previous_text, outgoing_opacity, 1.0, 0.0, 0.0, false);
                draw_layer(display_text, 1.0, 1.0, 0.0, 0.0, true);
                break;
            case 8: {
                const double shake = 15.0 * std::sin(raw_progress * 10.0 * kPi) * (1.0 - raw_progress);
                draw_layer(previous_text, outgoing_opacity, 1.0, -shake * 0.35, 0.0, false);
                draw_layer(display_text, incoming_opacity, 1.0, shake, 0.0, false);
                break;
            }
            case 9: // Flip
                if (progress < 0.5) {
                    draw_full_flip(previous_text, std::max(0.01, 1.0 - 2.0 * progress));
                } else {
                    draw_full_flip(display_text, std::max(0.01, 2.0 * progress - 1.0));
                }
                break;
            case 10: // Roll
                draw_layer(previous_text, 1.0, 1.0, 0.0, -static_cast<double>(kCanvasHeight) * progress, false);
                draw_layer(display_text, 1.0, 1.0, 0.0,
                           static_cast<double>(kCanvasHeight) * (1.0 - progress), false);
                break;
            case 11: {
                const double scale_from = static_cast<double>(custom_animation_scale_start_percent_) / 100.0;
                const double scale = scale_from + (1.0 - scale_from) * progress;
                const double offset_x = static_cast<double>(custom_animation_offset_x_) * (1.0 - progress);
                const double offset_y = static_cast<double>(custom_animation_offset_y_) * (1.0 - progress);
                const double rotation = static_cast<double>(custom_animation_rotation_) * (1.0 - progress);
                const double old_opacity = custom_animation_fade_ ? outgoing_opacity : 1.0;
                const double new_opacity = custom_animation_fade_ ? incoming_opacity : 1.0;
                draw_layer(previous_text, old_opacity, 1.0, -offset_x * 0.35, -offset_y * 0.35, false,
                           -rotation * 0.35);
                draw_layer(display_text, new_opacity, scale, offset_x, offset_y, false, rotation);
                break;
            }
            default:
                draw_layer(display_text, 1.0, 1.0, 0.0, 0.0, false);
                break;
            }
        }

        painter.end();

        last_display_seconds_ = display_seconds(timer_.remaining());
        last_rendered_text_ = current_text_;
        texture_dirty_ = true;
    }

    int64_t duration_seconds_;
    int display_mode_;
    int clock_format_;
    bool auto_start_;
    bool start_on_stream_start_;
    bool show_background_;
    bool antialiasing_;
    uint32_t text_color_;
    uint32_t background_color_;
    uint32_t clock_text_color_{kDefaultTextColor};
    int clock_font_size_percent_{100};
    bool clock_frame_enabled_{false};
    uint32_t clock_frame_color_{0x99000000};
    int clock_frame_shape_{kClockFrameRounded};
    int clock_frame_radius_{28};
    int clock_frame_padding_{24};
    uint32_t clock_frame_border_color_{kDefaultTextColor};
    int clock_frame_border_width_{0};
    std::string clock_font_face_{"Arial"};
    std::string clock_font_style_{"Regular"};
    std::string clock_custom_font_path_;
    std::string clock_custom_font_family_;
    int clock_custom_font_id_{-1};
    int clock_font_size_points_{144};
    uint32_t clock_font_flags_{0};
    int font_size_percent_;
    int time_format_;
    std::string end_message_;
    bool switch_scene_on_end_;
    int end_scene_delay_seconds_;
    std::string end_scene_name_;
    double end_scene_elapsed_seconds_{0.0};
    bool end_scene_fired_{false};
    std::array<MomentRule, kMomentCount> moments_{};
    std::array<bool, kMomentCount> moment_fired_{};
    std::string moment_overlay_text_;
    double moment_overlay_remaining_seconds_{0.0};
    int start_animation_;
    int change_animation_;
    int animation_duration_ms_;
    int animation_easing_;
    bool animate_digits_only_;
    int animation_stagger_ms_;
    int custom_animation_scale_start_percent_;
    int custom_animation_offset_x_;
    int custom_animation_offset_y_;
    int custom_animation_rotation_;
    bool custom_animation_fade_;
    int active_animation_preset_{0};
    bool animation_active_{false};
    bool active_animation_is_change_{false};
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
    std::string last_rendered_text_;
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

    obs_data_t *clock_font = obs_data_create();
    obs_data_set_default_string(clock_font, "face", "Arial");
    obs_data_set_default_string(clock_font, "style", "Regular");
    obs_data_set_default_int(clock_font, "size", 144);
    obs_data_set_default_int(clock_font, "flags", 0);
    obs_data_set_default_obj(settings, "clock_font", clock_font);
    obs_data_release(clock_font);
    obs_data_set_default_string(settings, "clock_custom_font_file", "");
    obs_data_set_default_int(settings, "duration_seconds", kDefaultDurationSeconds);
    obs_data_set_default_string(settings, "live_preview", "");
    obs_data_set_default_int(settings, "display_mode", kDisplayModeCountdown);
    obs_data_set_default_int(settings, "clock_format", 0);
    obs_data_set_default_int(settings, "clock_text_color", kDefaultTextColor);
    obs_data_set_default_int(settings, "clock_font_size_percent", 100);
    obs_data_set_default_bool(settings, "clock_frame_enabled", false);
    obs_data_set_default_int(settings, "clock_frame_color", 0x99000000);
    obs_data_set_default_int(settings, "clock_frame_shape", kClockFrameRounded);
    obs_data_set_default_int(settings, "clock_frame_radius", 28);
    obs_data_set_default_int(settings, "clock_frame_padding", 24);
    obs_data_set_default_int(settings, "clock_frame_border_color", kDefaultTextColor);
    obs_data_set_default_int(settings, "clock_frame_border_width", 0);
    obs_data_set_default_bool(settings, "auto_start", true);
    obs_data_set_default_bool(settings, "show_background", false);
    obs_data_set_default_bool(settings, "antialiasing", true);
    obs_data_set_default_int(settings, "text_color", kDefaultTextColor);
    obs_data_set_default_int(settings, "background_color", kDefaultBackgroundColor);
    obs_data_set_default_int(settings, "font_size_percent", 100);
    obs_data_set_default_int(settings, "time_format", 0);
    obs_data_set_default_string(settings, "end_message", obs_module_text("Settings.DefaultEndMessage"));
    obs_data_set_default_bool(settings, "start_on_stream_start", false);
    obs_data_set_default_bool(settings, "reset_on_stream_start", false);
    obs_data_set_default_bool(settings, "switch_scene_on_end", false);
    obs_data_set_default_int(settings, "end_scene_delay_seconds", 0);
    obs_data_set_default_string(settings, "end_scene_name", "");
    for (int index = 0; index < kMomentCount; ++index) {
        const std::string prefix = "moment_" + std::to_string(index + 1);
        obs_data_set_default_bool(settings, (prefix + "_enabled").c_str(), false);
        obs_data_set_default_int(settings, (prefix + "_seconds").c_str(), index == 0 ? 60 : (index == 1 ? 10 : 5));
        obs_data_set_default_int(settings, (prefix + "_action").c_str(), kMomentShowMessage);
        obs_data_set_default_string(settings, (prefix + "_message").c_str(), obs_module_text("Settings.DefaultMomentMessage"));
        obs_data_set_default_int(settings, (prefix + "_message_duration_seconds").c_str(), 3);
        obs_data_set_default_string(settings, (prefix + "_scene").c_str(), "");
    }
    obs_data_set_default_int(settings, "start_animation", 1);
    obs_data_set_default_int(settings, "change_animation", 1);
    obs_data_set_default_int(settings, "animation_duration_ms", 350);
    obs_data_set_default_int(settings, "animation_easing", 1);
    obs_data_set_default_bool(settings, "animate_digits_only", true);
    obs_data_set_default_int(settings, "animation_stagger_ms", 40);
    obs_data_set_default_int(settings, "animation_custom_scale_start_percent", 85);
    obs_data_set_default_int(settings, "animation_custom_offset_x", 0);
    obs_data_set_default_int(settings, "animation_custom_offset_y", 80);
    obs_data_set_default_int(settings, "animation_custom_rotation", 0);
    obs_data_set_default_bool(settings, "animation_custom_fade", true);
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

void set_display_mode_visibility(obs_properties_t *properties, int display_mode)
{
    const bool show_countdown = display_mode == kDisplayModeCountdown;
    const char *countdown_groups[] = {
        "general_group", "start_settings_group", "end_settings_group", "display_format_group", "moments_group",
        "appearance_group"};
    for (const char *name : countdown_groups) {
        obs_property_t *property = obs_properties_get(properties, name);
        if (property != nullptr) {
            obs_property_set_visible(property, show_countdown);
        }
    }
    const char *clock_groups[] = {"clock_format_group", "clock_appearance_group"};
    for (const char *name : clock_groups) {
        obs_property_t *property = obs_properties_get(properties, name);
        if (property != nullptr) {
            obs_property_set_visible(property, !show_countdown);
        }
    }
}

void refresh_live_preview(obs_properties_t *properties, obs_data_t *settings)
{
    while (obs_properties_get_parent(properties) != nullptr) {
        properties = obs_properties_get_parent(properties);
    }
    obs_property_t *preview = obs_properties_get(properties, "live_preview");
    if (preview != nullptr) {
        const QByteArray markup = live_preview_markup(settings).toUtf8();
        obs_property_set_description(preview, markup.constData());
    }
}

bool display_mode_modified(obs_properties_t *properties, obs_property_t *, obs_data_t *settings)
{
    const int mode = static_cast<int>(std::clamp<int64_t>(
        obs_data_get_int(settings, "display_mode"), kDisplayModeCountdown, kDisplayModeClock));
    set_display_mode_visibility(properties, mode);
    refresh_live_preview(properties, settings);
    return true;
}

bool live_preview_modified(obs_properties_t *properties, obs_property_t *, obs_data_t *settings)
{
    refresh_live_preview(properties, settings);
    return true;
}

obs_properties_t *source_properties(void *data)
{
    auto *properties = obs_properties_create();

    auto *preview = obs_properties_add_text(properties, "live_preview", obs_module_text("Settings.LivePreview"),
                                            OBS_TEXT_INFO);
    obs_property_text_set_info_word_wrap(preview, true);

    auto *mode = obs_properties_add_list(properties, "display_mode", obs_module_text("Settings.DisplayMode"),
                                         OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(mode, obs_module_text("DisplayMode.Countdown"), kDisplayModeCountdown);
    obs_property_list_add_int(mode, obs_module_text("DisplayMode.Clock"), kDisplayModeClock);
    obs_property_set_modified_callback(mode, display_mode_modified);

    auto *general = obs_properties_create();
    auto *duration = obs_properties_add_int(general, "duration_seconds", obs_module_text("Settings.Duration"), 1,
                                            static_cast<int>(kMaximumDurationSeconds), 1);
    obs_property_set_modified_callback(duration, live_preview_modified);
    obs_properties_add_bool(general, "auto_start", obs_module_text("Settings.AutoStart"));
    obs_properties_add_group(properties, "general_group", obs_module_text("Settings.GeneralGroup"),
                             OBS_GROUP_NORMAL, general);

    auto *start_settings = obs_properties_create();
    obs_properties_add_bool(start_settings, "start_on_stream_start", obs_module_text("Settings.StartOnStreamStart"));
    obs_properties_add_bool(start_settings, "reset_on_stream_start", obs_module_text("Settings.ResetOnStreamStart"));
    obs_properties_add_group(properties, "start_settings_group", obs_module_text("Settings.StartSettingsGroup"),
                             OBS_GROUP_NORMAL, start_settings);

    auto *end_settings = obs_properties_create();
    obs_properties_add_text(end_settings, "end_message", obs_module_text("Settings.EndMessage"), OBS_TEXT_DEFAULT);
    obs_properties_add_bool(end_settings, "switch_scene_on_end", obs_module_text("Settings.SwitchSceneOnEnd"));
    obs_properties_add_int(end_settings, "end_scene_delay_seconds", obs_module_text("Settings.EndSceneDelay"), 0, 3600, 1);
    add_scene_list(end_settings, "end_scene_name", obs_module_text("Settings.EndScene"));
    obs_properties_add_group(properties, "end_settings_group", obs_module_text("Settings.EndSettingsGroup"),
                             OBS_GROUP_NORMAL, end_settings);

    auto *display_format = obs_properties_create();
    auto *format = obs_properties_add_list(display_format, "time_format", obs_module_text("Settings.TimeFormat"),
                                           OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_set_modified_callback(format, live_preview_modified);
    obs_property_list_add_int(format, obs_module_text("Format.Automatic"), 0);
    obs_property_list_add_int(format, "MM:SS", 1);
    obs_property_list_add_int(format, "HH:MM:SS", 2);
    obs_properties_add_group(properties, "display_format_group", obs_module_text("Settings.DisplayFormatGroup"),
                             OBS_GROUP_NORMAL, display_format);

    auto *clock_settings = obs_properties_create();
    auto *clock_format = obs_properties_add_list(clock_settings, "clock_format",
                                                  obs_module_text("Settings.ClockFormat"),
                                                  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_set_modified_callback(clock_format, live_preview_modified);
    obs_property_list_add_int(clock_format, obs_module_text("Clock.Format.HoursMinutes"), 0);
    obs_property_list_add_int(clock_format, obs_module_text("Clock.Format.HoursMinutesSeconds"), 1);
    obs_properties_add_group(properties, "clock_format_group", obs_module_text("Settings.ClockSettingsGroup"),
                             OBS_GROUP_NORMAL, clock_settings);

    auto *clock_appearance = obs_properties_create();
    auto *clock_text_color = obs_properties_add_color_alpha(
        clock_appearance, "clock_text_color", obs_module_text("Settings.ClockTextColor"));
    obs_property_set_modified_callback(clock_text_color, live_preview_modified);
    auto *clock_font = obs_properties_add_font(clock_appearance, "clock_font", obs_module_text("Settings.ClockFont"));
    obs_property_set_modified_callback(clock_font, live_preview_modified);
    auto *clock_custom_font = obs_properties_add_path(
        clock_appearance, "clock_custom_font_file", obs_module_text("Settings.ClockCustomFontFile"),
        OBS_PATH_FILE, "Font files (*.ttf *.otf *.ttc)", nullptr);
    obs_property_set_modified_callback(clock_custom_font, live_preview_modified);
    auto *clock_font_size = obs_properties_add_int_slider(
        clock_appearance, "clock_font_size_percent", obs_module_text("Settings.ClockFontSize"), 25, 100, 1);
    obs_property_set_modified_callback(clock_font_size, live_preview_modified);
    auto *clock_frame_enabled = obs_properties_add_bool(
        clock_appearance, "clock_frame_enabled", obs_module_text("Settings.ClockFrameEnabled"));
    obs_property_set_modified_callback(clock_frame_enabled, live_preview_modified);
    auto *clock_frame_color = obs_properties_add_color_alpha(
        clock_appearance, "clock_frame_color", obs_module_text("Settings.ClockFrameColor"));
    obs_property_set_modified_callback(clock_frame_color, live_preview_modified);
    auto *clock_frame_shape = obs_properties_add_list(
        clock_appearance, "clock_frame_shape", obs_module_text("Settings.ClockFrameShape"),
        OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(clock_frame_shape, obs_module_text("Settings.ClockFrameRectangle"), kClockFrameRectangle);
    obs_property_list_add_int(clock_frame_shape, obs_module_text("Settings.ClockFrameRounded"), kClockFrameRounded);
    obs_property_list_add_int(clock_frame_shape, obs_module_text("Settings.ClockFramePill"), kClockFramePill);
    obs_property_list_add_int(clock_frame_shape, obs_module_text("Settings.ClockFrameEllipse"), kClockFrameEllipse);
    obs_property_set_modified_callback(clock_frame_shape, live_preview_modified);
    auto *clock_frame_radius = obs_properties_add_int_slider(
        clock_appearance, "clock_frame_radius", obs_module_text("Settings.ClockFrameRadius"), 0, 180, 1);
    obs_property_set_modified_callback(clock_frame_radius, live_preview_modified);
    auto *clock_frame_padding = obs_properties_add_int_slider(
        clock_appearance, "clock_frame_padding", obs_module_text("Settings.ClockFramePadding"), 0, 120, 1);
    obs_property_set_modified_callback(clock_frame_padding, live_preview_modified);
    auto *clock_frame_border_color = obs_properties_add_color_alpha(
        clock_appearance, "clock_frame_border_color", obs_module_text("Settings.ClockFrameBorderColor"));
    obs_property_set_modified_callback(clock_frame_border_color, live_preview_modified);
    auto *clock_frame_border_width = obs_properties_add_int_slider(
        clock_appearance, "clock_frame_border_width", obs_module_text("Settings.ClockFrameBorderWidth"), 0, 20, 1);
    obs_property_set_modified_callback(clock_frame_border_width, live_preview_modified);
    obs_properties_add_group(properties, "clock_appearance_group", obs_module_text("Settings.ClockAppearanceGroup"),
                             OBS_GROUP_NORMAL, clock_appearance);

    auto *rendering_settings = obs_properties_create();
    auto *antialiasing = obs_properties_add_bool(rendering_settings, "antialiasing",
                                                  obs_module_text("Settings.Antialiasing"));
    obs_property_set_modified_callback(antialiasing, live_preview_modified);
    obs_properties_add_group(properties, "rendering_group", obs_module_text("Settings.RenderingGroup"),
                             OBS_GROUP_NORMAL, rendering_settings);

    auto *appearance = obs_properties_create();
    auto *show_background = obs_properties_add_bool(appearance, "show_background",
                                                     obs_module_text("Settings.ShowBackground"));
    obs_property_set_modified_callback(show_background, live_preview_modified);
    auto *text_color = obs_properties_add_color_alpha(appearance, "text_color", obs_module_text("Settings.TextColor"));
    obs_property_set_modified_callback(text_color, live_preview_modified);
    auto *background_color = obs_properties_add_color_alpha(
        appearance, "background_color", obs_module_text("Settings.BackgroundColor"));
    obs_property_set_modified_callback(background_color, live_preview_modified);
    auto *font_property = obs_properties_add_font(appearance, "font", obs_module_text("Settings.Font"));
    obs_property_set_modified_callback(font_property, live_preview_modified);
    auto *custom_font = obs_properties_add_path(appearance, "custom_font_file",
                                                obs_module_text("Settings.CustomFontFile"),
                                                OBS_PATH_FILE, "Font files (*.ttf *.otf *.ttc)", nullptr);
    obs_property_set_modified_callback(custom_font, live_preview_modified);
    auto *font_size = obs_properties_add_int_slider(appearance, "font_size_percent",
                                                   obs_module_text("Settings.FontSize"), 25, 100, 1);
    obs_property_set_modified_callback(font_size, live_preview_modified);
    obs_properties_add_group(properties, "appearance_group", obs_module_text("Settings.AppearanceGroup"),
                             OBS_GROUP_NORMAL, appearance);

    auto *moments = obs_properties_create();
    for (int index = 0; index < kMomentCount; ++index) {
        const std::string prefix = "moment_" + std::to_string(index + 1);
        auto *moment = obs_properties_create();
        obs_properties_add_bool(moment, (prefix + "_enabled").c_str(), obs_module_text("Settings.MomentEnabled"));
        obs_properties_add_int(moment, (prefix + "_seconds").c_str(), obs_module_text("Settings.MomentAtSeconds"),
                               1, static_cast<int>(kMaximumDurationSeconds), 1);
        auto *action = obs_properties_add_list(moment, (prefix + "_action").c_str(),
                                               obs_module_text("Settings.MomentAction"),
                                               OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
        obs_property_list_add_int(action, obs_module_text("Moment.ActionMessage"), kMomentShowMessage);
        obs_property_list_add_int(action, obs_module_text("Moment.ActionPause"), kMomentPause);
        obs_property_list_add_int(action, obs_module_text("Moment.ActionAddMinute"), kMomentAddMinute);
        obs_property_list_add_int(action, obs_module_text("Moment.ActionScene"), kMomentSwitchScene);
        obs_properties_add_text(moment, (prefix + "_message").c_str(), obs_module_text("Settings.MomentMessage"),
                                OBS_TEXT_DEFAULT);
        obs_properties_add_int(moment, (prefix + "_message_duration_seconds").c_str(),
                               obs_module_text("Settings.MomentMessageDuration"), 1, 30, 1);
        add_scene_list(moment, (prefix + "_scene").c_str(), obs_module_text("Settings.MomentScene"));
        const std::string group_name = prefix + "_group";
        const std::string group_label = std::string(obs_module_text("Settings.MomentGroup")) + " "
                                        + std::to_string(index + 1);
        obs_properties_add_group(moments, group_name.c_str(), group_label.c_str(), OBS_GROUP_NORMAL, moment);
    }
    obs_properties_add_group(properties, "moments_group", obs_module_text("Settings.KeyMomentsGroup"),
                             OBS_GROUP_NORMAL, moments);

    auto *animation = obs_properties_create();
    auto *start_animation = obs_properties_add_list(animation, "start_animation",
                                                    obs_module_text("Settings.StartAnimation"),
                                                    OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    auto *change_animation = obs_properties_add_list(animation, "change_animation",
                                                     obs_module_text("Settings.ChangeAnimation"),
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    const char *animation_names[] = {
        "Animation.None", "Animation.Fade", "Animation.SlideUp", "Animation.SlideLeft",
        "Animation.Zoom", "Animation.Bounce", "Animation.Pulse", "Animation.Reveal", "Animation.Shake",
        "Animation.Flip", "Animation.Roll", "Animation.Custom"};
    for (int preset = 0; preset <= 11; ++preset) {
        const char *label = obs_module_text(animation_names[preset]);
        obs_property_list_add_int(start_animation, label, preset);
        obs_property_list_add_int(change_animation, label, preset);
    }
    obs_properties_add_bool(animation, "animate_digits_only", obs_module_text("Settings.AnimateDigitsOnly"));
    obs_properties_add_int_slider(animation, "animation_duration_ms",
                                  obs_module_text("Settings.AnimationDuration"), 100, 3000, 50);
    obs_properties_add_int_slider(animation, "animation_stagger_ms",
                                  obs_module_text("Settings.AnimationStagger"), 0, 250, 10);
    auto *easing = obs_properties_add_list(animation, "animation_easing", obs_module_text("Settings.AnimationEasing"),
                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(easing, obs_module_text("Animation.EasingLinear"), 0);
    obs_property_list_add_int(easing, obs_module_text("Animation.EasingOut"), 1);
    obs_property_list_add_int(easing, obs_module_text("Animation.EasingInOut"), 2);

    auto *custom_animation = obs_properties_create();
    obs_properties_add_int_slider(custom_animation, "animation_custom_scale_start_percent",
                                  obs_module_text("Settings.CustomAnimationScale"), 25, 200, 1);
    obs_properties_add_int_slider(custom_animation, "animation_custom_offset_x",
                                  obs_module_text("Settings.CustomAnimationOffsetX"), -300, 300, 1);
    obs_properties_add_int_slider(custom_animation, "animation_custom_offset_y",
                                  obs_module_text("Settings.CustomAnimationOffsetY"), -300, 300, 1);
    obs_properties_add_int_slider(custom_animation, "animation_custom_rotation",
                                  obs_module_text("Settings.CustomAnimationRotation"), -180, 180, 1);
    obs_properties_add_bool(custom_animation, "animation_custom_fade", obs_module_text("Settings.CustomAnimationFade"));
    obs_properties_add_group(animation, "custom_animation_group", obs_module_text("Settings.CustomAnimationGroup"),
                             OBS_GROUP_NORMAL, custom_animation);
    obs_properties_add_group(properties, "animation_group", obs_module_text("Settings.AnimationGroup"),
                             OBS_GROUP_NORMAL, animation);

    auto *about_author = obs_properties_add_text(
        properties, "about_author", (std::string(obs_module_text("About.By")) + " Haribou_").c_str(), OBS_TEXT_INFO);
    obs_property_text_set_info_word_wrap(about_author, true);
    auto *about_version = obs_properties_add_text(
        properties, "about_version",
        (std::string(obs_module_text("About.Version")) + " " COUNTDOWNSTUDIO_VERSION).c_str(), OBS_TEXT_INFO);
    obs_property_text_set_info_word_wrap(about_version, true);

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
    set_display_mode_visibility(properties, data != nullptr
                                                 ? static_cast<CountdownSource *>(data)->display_mode()
                                                 : kDisplayModeCountdown);
    if (data != nullptr) {
        obs_data_t *preview_settings = static_cast<CountdownSource *>(data)->preview_settings();
        const QByteArray markup = live_preview_markup(preview_settings).toUtf8();
        obs_property_set_description(preview, markup.constData());
        obs_data_release(preview_settings);
    }
    return properties;
}

} // namespace

void start_countdown_on_stream(obs_source_t *source)
{
    if (source == nullptr || std::strcmp(obs_source_get_id(source), "countdown_studio_timer") != 0) {
        return;
    }
    obs_data_t *settings = obs_source_get_settings(source);
    const bool start_on_live = obs_data_get_bool(settings, "start_on_stream_start");
    const bool reset_on_live = obs_data_get_bool(settings, "reset_on_stream_start");
    const int display_mode = static_cast<int>(obs_data_get_int(settings, "display_mode"));
    obs_data_release(settings);
    if (!start_on_live || display_mode != kDisplayModeCountdown) {
        return;
    }

    obs_properties_t *properties = obs_source_properties(source);
    if (properties != nullptr) {
        obs_property_t *button = obs_properties_get(properties, reset_on_live ? "restart" : "start");
        if (button != nullptr) {
            obs_property_button_clicked(button, nullptr);
        }
        obs_properties_destroy(properties);
    }
}

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
