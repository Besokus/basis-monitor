#include <cassert>
#include <chrono>

#include "basis_monitor/domain/alert_event.h"
#include "basis_monitor/monitor/alert_engine.h"

int main()
{
    using basis_monitor::AlertEngine;
    using basis_monitor::AlertTransition;
    using Clock = std::chrono::system_clock;

    Clock::time_point now = Clock::from_time_t(0);
    AlertEngine engine(
        -0.10,
        std::chrono::minutes(20),
        [&now]() {
            return now;
        });

    const auto first = engine.Evaluate("IC2604", -1.25);
    assert(first.transition == AlertTransition::EnteredNegative);

    const auto repeat = engine.Evaluate("IC2604", -0.50);
    assert(repeat.transition == AlertTransition::None);

    now += std::chrono::minutes(19);
    const auto suppressed = engine.Evaluate("IC2604", -0.50);
    assert(suppressed.transition == AlertTransition::None);

    now += std::chrono::minutes(1);
    const auto reminder = engine.Evaluate("IC2604", -0.50);
    assert(reminder.transition == AlertTransition::RepeatedNegative);

    now += std::chrono::minutes(1);
    const auto short_recovery = engine.Evaluate("IC2604", 0.80);
    assert(short_recovery.transition == AlertTransition::None);

    const auto negative_again_within_window = engine.Evaluate("IC2604", -0.20);
    assert(negative_again_within_window.transition == AlertTransition::None);

    now += std::chrono::minutes(20);
    const auto long_recovery = engine.Evaluate("IC2604", 0.80);
    assert(long_recovery.transition == AlertTransition::None);

    const auto negative_again_after_window = engine.Evaluate("IC2604", -0.20);
    assert(negative_again_after_window.transition == AlertTransition::EnteredNegative);

    const auto threshold_normal = engine.Evaluate("IC2605", -0.05);
    assert(threshold_normal.transition == AlertTransition::None);

    const auto normal = engine.Evaluate("IC2606", 1.10);
    assert(normal.transition == AlertTransition::None);

    return 0;
}
