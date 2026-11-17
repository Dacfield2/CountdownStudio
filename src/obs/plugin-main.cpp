#include <obs-module.h>

#include "countdown-source.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("CountdownStudio", "en-US")

bool obs_module_load(void)
{
    countdownstudio::register_countdown_source();
    blog(LOG_INFO, "[Countdown Studio] countdown source registered.");
    return true;
}
