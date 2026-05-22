#include "basis_monitor/monitor/alert_engine.h"

namespace basis_monitor
{

AlertEngine::AlertEngine(double negative_threshold,
                         std::chrono::minutes repeat_interval,
                         Clock clock)
    : negative_threshold_(negative_threshold),
      repeat_interval_(repeat_interval),
      clock_(std::move(clock))
{
    if (!clock_)
    {
        clock_ = [] {
            return std::chrono::system_clock::now();
        };
    }
}

AlertEvent AlertEngine::Evaluate(const std::string& instrument_id, double annual_rate)
{
    const bool is_negative = annual_rate < negative_threshold_;
    auto& state = state_[instrument_id];
    const bool was_negative = state.is_negative;
    const auto now = clock_();

    AlertEvent event = {};
    event.instrument_id = instrument_id;
    event.annual_rate = annual_rate;

    if (is_negative && !was_negative)
    {
        event.transition = AlertTransition::EnteredNegative;
        state.last_alert_time = now;
        state.has_alert_time = true;
    }
    else if (is_negative && was_negative)
    {
        if (!state.has_alert_time || now - state.last_alert_time >= repeat_interval_)
        {
            event.transition = AlertTransition::RepeatedNegative;
            state.last_alert_time = now;
            state.has_alert_time = true;
        }
    }

    if (is_negative)
    {
        state.is_negative = true;
    }
    else if (!state.has_alert_time || now - state.last_alert_time >= repeat_interval_)
    {
        state.is_negative = false;
        state.has_alert_time = false;
    }
    return event;
}

} // namespace basis_monitor
