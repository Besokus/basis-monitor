#pragma once

#include <memory>

#include "basis_monitor/config/app_config.h"

namespace basis_monitor
{

class MdListener;
class IMarketDataSession;

std::unique_ptr<IMarketDataSession> CreateMarketDataSession(const AppConfig& config, MdListener& listener);

} // namespace basis_monitor
