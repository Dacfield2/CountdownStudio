#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("CountdownStudio", "en-US")

bool obs_module_load(void)
{
    blog(LOG_INFO, "[Countdown Studio] OBS module loaded (bootstrap stage).");
    return true;
}
