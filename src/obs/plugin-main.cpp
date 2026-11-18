#include <obs-frontend-api.h>
#include <obs-module.h>

#include "countdown-dock.hpp"
#include "countdown-source.hpp"
#include "countdown-toolbar.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("CountdownStudio", "en-US")

namespace {

void frontend_event(enum obs_frontend_event event, void *)
{
    if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
        countdownstudio::register_countdown_dock();
        countdownstudio::register_countdown_toolbar();
    }
}

} // namespace

bool obs_module_load(void)
{
    countdownstudio::register_countdown_source();
    obs_frontend_add_event_callback(frontend_event, nullptr);
    countdownstudio::register_countdown_dock();
    countdownstudio::register_countdown_toolbar();
    blog(LOG_INFO, "[Countdown Studio] countdown source registered.");
    return true;
}

void obs_module_unload(void)
{
    obs_frontend_remove_event_callback(frontend_event, nullptr);
    countdownstudio::unregister_countdown_toolbar();
    countdownstudio::unregister_countdown_dock();
}
