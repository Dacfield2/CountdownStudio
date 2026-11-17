#include "countdownstudio/countdown.hpp"

#include <functional>
#include <iostream>
#include <string_view>

using countdownstudio::Countdown;
using namespace std::chrono_literals;

namespace {

int failures = 0;

void check(bool condition, std::string_view message)
{
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void run(std::string_view name, const std::function<void()> &test)
{
    const int before = failures;
    test();
    if (failures == before) {
        std::cout << "PASS: " << name << '\n';
    }
}

void idle_countdown_reports_full_duration()
{
    const auto start = Countdown::Clock::time_point{} + 10s;
    const Countdown timer{90s};
    check(timer.state(start) == Countdown::State::idle, "new timer should be idle");
    check(timer.remaining(start) == 90s, "idle timer should report its full duration");
}

void countdown_runs_to_zero()
{
    const auto start = Countdown::Clock::time_point{} + 10s;
    Countdown timer{5s};
    timer.start(start);

    check(timer.state(start + 2s) == Countdown::State::running, "timer should run before its deadline");
    check(timer.remaining(start + 2s) == 3s, "remaining duration should decrease while running");
    check(timer.state(start + 5s) == Countdown::State::finished, "timer should finish at its deadline");
    check(timer.remaining(start + 5s) == 0ms, "finished timer should report zero");
}

void pause_and_resume_preserve_remaining_time()
{
    const auto start = Countdown::Clock::time_point{} + 20s;
    Countdown timer{10s};
    timer.start(start);
    timer.pause(start + 4s);

    check(timer.state(start + 4s) == Countdown::State::paused, "pause should change the state");
    check(timer.remaining(start + 100s) == 6s, "paused timer should keep its remaining duration");

    timer.resume(start + 100s);
    check(timer.state(start + 100s) == Countdown::State::running, "resume should restart the timer");
    check(timer.remaining(start + 103s) == 3s, "resumed deadline should use the saved remainder");
}

void reset_restarts_with_a_new_duration()
{
    const auto start = Countdown::Clock::time_point{} + 30s;
    Countdown timer{10s};
    timer.start(start);
    timer.reset(2s);

    check(timer.state(start + 1s) == Countdown::State::idle, "reset should return to idle");
    check(timer.remaining(start + 1s) == 2s, "reset should store the new duration");
    timer.start(start + 1s);
    check(timer.remaining(start + 2s) == 1s, "new duration should count down after start");
}

void zero_duration_finishes_immediately()
{
    const auto start = Countdown::Clock::time_point{} + 40s;
    Countdown timer{0ms};
    timer.start(start);
    check(timer.state(start) == Countdown::State::finished, "zero duration should finish immediately");
    check(timer.remaining(start) == 0ms, "zero duration should report zero");
}

void starting_after_finish_restarts_the_duration()
{
    const auto start = Countdown::Clock::time_point{} + 50s;
    Countdown timer{2s};
    timer.start(start);
    check(timer.state(start + 2s) == Countdown::State::finished, "timer should finish before restart");

    timer.start(start + 10s);
    check(timer.state(start + 10s) == Countdown::State::running, "start should restart a finished timer");
    check(timer.remaining(start + 11s) == 1s, "restarted timer should count from the full duration");
}

void added_time_extends_a_running_countdown()
{
    const auto start = Countdown::Clock::time_point{} + 70s;
    Countdown timer{10s};
    timer.start(start);
    timer.add_time(2min, start + 4s);

    check(timer.remaining(start + 4s) == 126s, "adding time should extend a running deadline");
    check(timer.remaining(start + 126s) == 4s, "added time should remain part of the running countdown");
}

void added_time_extends_a_paused_countdown()
{
    const auto start = Countdown::Clock::time_point{} + 80s;
    Countdown timer{10s};
    timer.start(start);
    timer.pause(start + 4s);
    timer.add_time(3min, start + 100s);

    check(timer.state(start + 100s) == Countdown::State::paused, "adding time should keep a paused timer paused");
    check(timer.remaining(start + 100s) == 186s, "adding time should increase a paused remainder");
}

void added_time_restarts_a_finished_countdown()
{
    const auto start = Countdown::Clock::time_point{} + 90s;
    Countdown timer{2s};
    timer.start(start);
    timer.add_time(1min, start + 2s);

    check(timer.state(start + 2s) == Countdown::State::running, "adding time after finish should restart the timer");
    check(timer.remaining(start + 2s) == 60s, "a finished timer should resume with the added duration");
}

void negative_duration_is_clamped_to_zero()
{
    const auto now = Countdown::Clock::time_point{} + 60s;
    Countdown timer{-1s};
    check(timer.remaining(now) == 0ms, "negative duration should be clamped to zero");
    timer.start(now);
    check(timer.state(now) == Countdown::State::finished, "clamped zero duration should finish immediately");
}

} // namespace

int main()
{
    run("idle duration", idle_countdown_reports_full_duration);
    run("running countdown", countdown_runs_to_zero);
    run("pause and resume", pause_and_resume_preserve_remaining_time);
    run("reset duration", reset_restarts_with_a_new_duration);
    run("zero duration", zero_duration_finishes_immediately);
    run("restart after finish", starting_after_finish_restarts_the_duration);
    run("add time while running", added_time_extends_a_running_countdown);
    run("add time while paused", added_time_extends_a_paused_countdown);
    run("add time after finish", added_time_restarts_a_finished_countdown);
    run("negative duration", negative_duration_is_clamped_to_zero);

    if (failures != 0) {
        std::cerr << failures << " check(s) failed\n";
        return 1;
    }

    std::cout << "All countdown core checks passed.\n";
    return 0;
}
