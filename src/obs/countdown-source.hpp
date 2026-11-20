#pragma once

#include <obs.h>
#include <obs-source.h>

namespace countdownstudio {

void register_countdown_source();
void invoke_countdown_control(obs_source_t *source, const char *property_name, int repetitions = 1);
void start_countdown_on_stream(obs_source_t *source);

} // namespace countdownstudio
