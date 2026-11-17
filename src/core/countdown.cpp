#include "countdownstudio/countdown.hpp"

namespace countdownstudio {

Countdown::Countdown(Duration duration) noexcept
{
    reset(duration);
}

void Countdown::start(Clock::time_point now) noexcept
{
    if (state_ == State::paused) {
        resume(now);
        return;
    }

    if (state_ != State::idle && state_ != State::finished) {
        return;
    }

    if (duration_ <= Duration::zero()) {
        state_ = State::finished;
        paused_remaining_ = Duration::zero();
        return;
    }

    deadline_ = now + duration_;
    paused_remaining_ = duration_;
    state_ = State::running;
}

void Countdown::pause(Clock::time_point now) noexcept
{
    if (state_ != State::running) {
        return;
    }

    paused_remaining_ = remaining(now);
    if (paused_remaining_ <= Duration::zero()) {
        paused_remaining_ = Duration::zero();
        state_ = State::finished;
        return;
    }

    state_ = State::paused;
}

void Countdown::resume(Clock::time_point now) noexcept
{
    if (state_ != State::paused) {
        return;
    }

    if (paused_remaining_ <= Duration::zero()) {
        paused_remaining_ = Duration::zero();
        state_ = State::finished;
        return;
    }

    deadline_ = now + paused_remaining_;
    state_ = State::running;
}

void Countdown::add_time(Duration extra, Clock::time_point now) noexcept
{
    if (extra <= Duration::zero()) {
        return;
    }

    switch (state(now)) {
    case State::idle:
        duration_ += extra;
        paused_remaining_ = duration_;
        break;
    case State::running:
        deadline_ += extra;
        duration_ += extra;
        break;
    case State::paused:
        paused_remaining_ += extra;
        duration_ += extra;
        break;
    case State::finished:
        duration_ += extra;
        paused_remaining_ = extra;
        deadline_ = now + extra;
        state_ = State::running;
        break;
    }
}

void Countdown::reset(Duration duration) noexcept
{
    duration_ = duration < Duration::zero() ? Duration::zero() : duration;
    paused_remaining_ = duration_;
    deadline_ = Clock::time_point{};
    state_ = State::idle;
}

Countdown::State Countdown::state(Clock::time_point now) const noexcept
{
    if (state_ == State::running && remaining(now) <= Duration::zero()) {
        return State::finished;
    }

    return state_;
}

Countdown::Duration Countdown::remaining(Clock::time_point now) const noexcept
{
    switch (state_) {
    case State::idle:
        return duration_;
    case State::paused:
        return paused_remaining_;
    case State::finished:
        return Duration::zero();
    case State::running:
        if (deadline_ <= now) {
            return Duration::zero();
        }
        return std::chrono::duration_cast<Duration>(deadline_ - now);
    }

    return Duration::zero();
}

} // namespace countdownstudio
