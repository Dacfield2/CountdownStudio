#include "countdown-toolbar.hpp"

#include "countdown-source.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.h>
#include <obs-source.h>

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLayout>
#include <QPushButton>
#include <QSizePolicy>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

#include <cstring>
#include <string>

namespace countdownstudio {
namespace {

constexpr char kFiltersButtonName[] = "sourceFiltersButton";
constexpr char kTimerSourceId[] = "countdown_studio_timer";
QWidget *toolbar_widget = nullptr;

struct SelectedTimer {
    obs_source_t *source{nullptr};
    int selected_count{0};
};

bool find_selected_timer(obs_scene_t *, obs_sceneitem_t *item, void *data)
{
    if (!obs_sceneitem_selected(item)) {
        return true;
    }

    auto *selection = static_cast<SelectedTimer *>(data);
    ++selection->selected_count;
    if (selection->selected_count > 1) {
        return false;
    }

    obs_source_t *source = obs_sceneitem_get_source(item);
    if (source != nullptr && std::strcmp(obs_source_get_id(source), kTimerSourceId) == 0) {
        selection->source = obs_source_get_ref(source);
    }
    return true;
}

obs_source_t *get_selected_timer()
{
    obs_source_t *scene_source = obs_frontend_preview_program_mode_active()
                                     ? obs_frontend_get_current_preview_scene()
                                     : obs_frontend_get_current_scene();
    if (scene_source == nullptr) {
        return nullptr;
    }

    SelectedTimer selection;
    obs_scene_t *scene = obs_scene_from_source(scene_source);
    if (scene != nullptr) {
        obs_scene_enum_items(scene, find_selected_timer, &selection);
    }
    obs_source_release(scene_source);

    if (selection.selected_count != 1) {
        if (selection.source != nullptr) {
            obs_source_release(selection.source);
        }
        return nullptr;
    }
    return selection.source;
}

class CountdownToolbar final : public QWidget {
public:
    explicit CountdownToolbar(QWidget *parent) : QWidget(parent)
    {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(1);

        add_button(layout, QStringLiteral("▶"), "Controls.Start", "start");
        add_button(layout, QStringLiteral("Ⅱ"), "Controls.Pause", "pause");
        add_button(layout, QStringLiteral("↻"), "Controls.Restart", "restart");
        add_button(layout, QStringLiteral("+1"), "Controls.AddMinute", "add_minute");
        add_button(layout, QStringLiteral("+5"), "Controls.AddFiveMinutes", "add_five_minutes");
        add_button(layout, QStringLiteral("+…"), "Controls.AddCustomMinutes", "custom");

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setToolTip(QString::fromUtf8(obs_module_text("Toolbar.Help")));

        auto *refresh = new QTimer(this);
        refresh->setInterval(300);
        QObject::connect(refresh, &QTimer::timeout, this, [this]() { refresh_selection(); });
        refresh->start();
        refresh_selection();
    }

private:
    void add_button(QHBoxLayout *layout, const QString &label, const char *tooltip_key,
                    const char *property_name)
    {
        auto *button = new QToolButton(this);
        button->setText(label);
        button->setToolTip(QString::fromUtf8(obs_module_text(tooltip_key)));
        button->setAccessibleName(button->toolTip());
        button->setAutoRaise(true);
        button->setFixedSize(26, 26);
        QObject::connect(button, &QToolButton::clicked, this, [this, property_name]() {
            if (std::strcmp(property_name, "custom") == 0) {
                add_custom_minutes();
            } else {
                trigger(property_name);
            }
        });
        layout->addWidget(button);
    }

    void refresh_selection()
    {
        obs_source_t *source = get_selected_timer();
        const bool enabled = source != nullptr;
        setVisible(enabled);
        for (auto *button : findChildren<QToolButton *>()) {
            button->setEnabled(enabled);
        }
        if (source != nullptr) {
            obs_source_release(source);
        }
    }

    void add_custom_minutes()
    {
        bool accepted = false;
        const int minutes = QInputDialog::getInt(
            this, QString::fromUtf8(obs_module_text("Controls.CustomMinutesTitle")),
            QString::fromUtf8(obs_module_text("Controls.CustomMinutesPrompt")), 10, 1, 120, 1, &accepted);
        if (accepted) {
            trigger("add_minute", minutes);
        }
    }

    void trigger(const char *property_name, int repetitions = 1)
    {
        obs_source_t *source = get_selected_timer();
        if (source == nullptr) {
            return;
        }

        invoke_countdown_control(source, property_name, repetitions);
        obs_source_release(source);
    }
};

} // namespace

void register_countdown_toolbar()
{
    if (toolbar_widget != nullptr) {
        return;
    }

    auto *main_window = static_cast<QWidget *>(obs_frontend_get_main_window());
    if (main_window == nullptr) {
        blog(LOG_WARNING, "[CountdownStudio] Could not find the OBS main window for source controls.");
        return;
    }

    QWidget *filters_button = main_window->findChild<QWidget *>(QString::fromUtf8(kFiltersButtonName));
    if (filters_button == nullptr || filters_button->parentWidget() == nullptr) {
        blog(LOG_WARNING, "[CountdownStudio] OBS source toolbar is unavailable; use the Countdown Studio dock.");
        return;
    }

    auto *layout = qobject_cast<QBoxLayout *>(filters_button->parentWidget()->layout());
    const int filters_index = layout != nullptr ? layout->indexOf(filters_button) : -1;
    if (layout == nullptr || filters_index < 0) {
        blog(LOG_WARNING, "[CountdownStudio] Could not place controls beside the OBS source buttons.");
        return;
    }

    auto *widget = new CountdownToolbar(filters_button->parentWidget());
    layout->insertWidget(filters_index + 1, widget);
    widget->show();
    toolbar_widget = widget;
}

void unregister_countdown_toolbar()
{
    if (toolbar_widget != nullptr) {
        if (toolbar_widget->parentWidget() != nullptr
            && toolbar_widget->parentWidget()->layout() != nullptr) {
            toolbar_widget->parentWidget()->layout()->removeWidget(toolbar_widget);
        }
        delete toolbar_widget;
        toolbar_widget = nullptr;
    }
}

} // namespace countdownstudio
