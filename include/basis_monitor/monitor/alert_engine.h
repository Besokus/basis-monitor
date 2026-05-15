#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

#include "basis_monitor/domain/alert_event.h"

namespace basis_monitor
{

class AlertEngine
{
public:
    using Clock = std::function<std::chrono::system_clock::time_point()>;

    AlertEngine(double negative_threshold = 0.0,
                std::chrono::minutes repeat_interval = std::chrono::minutes(20),
                Clock clock = {});

    AlertEvent Evaluate(const std::string& instrument_id, double annual_rate);

private:
    struct AlertState
    {
        bool is_negative = false;
        std::chrono::system_clock::time_point last_alert_time = {};
        bool has_alert_time = false;
    };

    double negative_threshold_ = 0.0;
    std::chrono::minutes repeat_interval_{20};
    Clock clock_;
    std::unordered_map<std::string, AlertState> state_;
};

} // namespace basis_monitor
