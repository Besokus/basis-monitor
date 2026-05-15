#pragma once

namespace basis_monitor
{

class IMarketDataSession
{
public:
    virtual ~IMarketDataSession() = default;

    virtual bool Start() = 0;
    virtual bool WaitForFirstMarketData(unsigned long timeout_ms) const = 0;
    virtual void Stop() = 0;
};

} // namespace basis_monitor
