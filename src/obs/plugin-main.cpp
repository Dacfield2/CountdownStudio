#include <obs-module.h>

#include "countdown-dock.hpp"
#include "countdown-source.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("CountdownStudio", "en-US")

bool obs_module_load(void)
{
    countdownstudio::register_countdown_source();
    countdownstudio::register_countdown_dock();
    blog(LOG_INFO, "[Countdown Studio] countdown source registered.");
    return true;
}

void obs_module_unload(void)
{
    countdownstudio::unregister_countdown_dock();
}
