#pragma once

#include <string>

#include "basis_monitor/config/app_config.h"

namespace basis_monitor
{

AppConfig LoadAppConfig(const std::string& config_path);

} // namespace basis_monitor
