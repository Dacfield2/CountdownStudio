#include "countdown-dock.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs-properties.h>
#include <obs-source.h>
#include <obs.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cstring>

namespace countdownstudio {
namespace {

constexpr char kDockId[] = "countdownstudio_control_dock";
constexpr char kSourceId[] = "countdown_studio_timer";
QWidget *dock_widget = nullptr;

bool collect_countdowns(void *data, obs_source_t *source)
{
    if (std::strcmp(obs_source_get_id(source), kSourceId) == 0) {
        auto *sources = static_cast<QComboBox *>(data);
        sources->addItem(QString::fromUtf8(obs_source_get_name(source)));
    }
    return true;
}

class CountdownDock final : public QWidget {
public:
    CountdownDock()
    {
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(QString::fromUtf8(obs_module_text("Dock.SelectSource")), this));

        sources_ = new QComboBox(this);
        layout->addWidget(sources_);

        auto *primary_row = new QHBoxLayout();
        add_button(primary_row, "Controls.Start", "start");
        add_button(primary_row, "Controls.Pause", "pause");
        layout->addLayout(primary_row);

        auto *secondary_row = new QHBoxLayout();
        add_button(secondary_row, "Controls.Restart", "restart");
        layout->addLayout(secondary_row);

        auto *time_row = new QHBoxLayout();
        add_button(time_row, "Controls.AddMinute", "add_minute");
        add_button(time_row, "Controls.AddFiveMinutes", "add_five_minutes");
        layout->addLayout(time_row);

        auto *custom_time_row = new QHBoxLayout();
        custom_time_row->addWidget(new QLabel(QString::fromUtf8(obs_module_text("Dock.MinutesToAdd")), this));
        custom_minutes_ = new QSpinBox(this);
        custom_minutes_->setRange(1, 120);
        custom_minutes_->setValue(10);
        custom_time_row->addWidget(custom_minutes_);
        auto *add_custom_time = new QPushButton(QString::fromUtf8(obs_module_text("Controls.AddCustomMinutes")), this);
        QObject::connect(add_custom_time, &QPushButton::clicked, this,
                         [this]() { trigger("add_minute", custom_minutes_->value()); });
        custom_time_row->addWidget(add_custom_time);
        layout->addLayout(custom_time_row);

        auto *configure = new QPushButton(QString::fromUtf8(obs_module_text("Dock.ConfigureSource")), this);
        QObject::connect(configure, &QPushButton::clicked, this, [this]() { open_source_properties(); });
        layout->addWidget(configure);

        status_ = new QLabel(QString::fromUtf8(obs_module_text("Dock.Help")), this);
        status_->setWordWrap(true);
        layout->addWidget(status_);
        layout->addStretch(1);

        auto *refresh = new QTimer(this);
        refresh->setInterval(1500);
        QObject::connect(refresh, &QTimer::timeout, this, [this]() { refresh_sources(); });
        refresh->start();
        refresh_sources();
    }

private:
    void add_button(QHBoxLayout *layout, const char *label, const char *property_name)
    {
        auto *button = new QPushButton(QString::fromUtf8(obs_module_text(label)), this);
        QObject::connect(button, &QPushButton::clicked, this, [this, property_name]() { trigger(property_name); });
        layout->addWidget(button);
    }

    void refresh_sources()
    {
        const auto previous = sources_->currentText();
        sources_->clear();
        obs_enum_sources(collect_countdowns, sources_);
        const int previous_index = sources_->findText(previous);
        if (previous_index >= 0) {
            sources_->setCurrentIndex(previous_index);
        }
        const bool has_source = sources_->count() > 0;
        for (auto *button : findChildren<QPushButton *>()) {
            button->setEnabled(has_source);
        }
        custom_minutes_->setEnabled(has_source);
        status_->setText(QString::fromUtf8(obs_module_text(has_source ? "Dock.Help" : "Dock.NoSources")));
    }

    void trigger(const char *property_name, int repetitions = 1)
    {
        const auto source_name = sources_->currentText().toUtf8();
        obs_source_t *source = obs_get_source_by_name(source_name.constData());
        if (source == nullptr) {
            return;
        }

        obs_properties_t *properties = obs_source_properties(source);
        if (properties != nullptr) {
            obs_property_t *property = obs_properties_get(properties, property_name);
            if (property != nullptr) {
                for (int index = 0; index < repetitions; ++index) {
                    obs_property_button_clicked(property, nullptr);
                }
            }
            obs_properties_destroy(properties);
        }
        obs_source_release(source);
    }

    void open_source_properties()
    {
        const auto source_name = sources_->currentText().toUtf8();
        obs_source_t *source = obs_get_source_by_name(source_name.constData());
        if (source != nullptr) {
            obs_frontend_open_source_properties(source);
            obs_source_release(source);
        }
    }

    QComboBox *sources_{nullptr};
    QSpinBox *custom_minutes_{nullptr};
    QLabel *status_{nullptr};
};

} // namespace

void register_countdown_dock()
{
    if (dock_widget != nullptr) {
        return;
    }

    auto *widget = new CountdownDock();
    if (obs_frontend_add_dock_by_id(kDockId, obs_module_text("Dock.Title"), widget)) {
        dock_widget = widget;
    } else {
        delete widget;
    }
}

void unregister_countdown_dock()
{
    if (dock_widget != nullptr) {
        obs_frontend_remove_dock(kDockId);
        dock_widget = nullptr;
    }
}

} // namespace countdownstudio
