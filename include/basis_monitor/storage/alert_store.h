#pragma once

#include <filesystem>
#include <string>

#include "basis_monitor/domain/alert_event.h"

namespace basis_monitor
{

class AlertStore
{
public:
    explicit AlertStore(std::filesystem::path file_path);

    bool Append(const AlertEvent& event, const std::string& reason_text);

private:
    std::filesystem::path file_path_;
};

} // namespace basis_monitor
