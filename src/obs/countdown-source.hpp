#pragma once

#include <obs.h>
#include <obs-source.h>

namespace countdownstudio {

void register_countdown_source();
void start_countdown_on_stream(obs_source_t *source);

} // namespace countdownstudio
