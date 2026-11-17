#pragma once

#include <chrono>

namespace countdownstudio {

class Countdown final {
public:
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;

    enum class State {
        idle,
        running,
        paused,
        finished,
    };

    explicit Countdown(Duration duration) noexcept;

    void start(Clock::time_point now = Clock::now()) noexcept;
    void pause(Clock::time_point now = Clock::now()) noexcept;
    void resume(Clock::time_point now = Clock::now()) noexcept;
    void add_time(Duration extra, Clock::time_point now = Clock::now()) noexcept;
    void reset(Duration duration) noexcept;

    [[nodiscard]] State state(Clock::time_point now = Clock::now()) const noexcept;
    [[nodiscard]] Duration remaining(Clock::time_point now = Clock::now()) const noexcept;

private:
    Duration duration_{};
    Duration paused_remaining_{};
    Clock::time_point deadline_{};
    State state_{State::idle};
};

} // namespace countdownstudio
