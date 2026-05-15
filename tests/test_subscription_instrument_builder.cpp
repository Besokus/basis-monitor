#include <cassert>
#include <vector>

#include "basis_monitor/data/subscription_instrument_builder.h"
#include "basis_monitor/domain/monitored_contract.h"

int main()
{
    {
        const std::vector<basis_monitor::MonitoredContract> monitored_contracts = {
            {"IF2606", "IF", "hs300", "000300.XSHG", "沪深300", 0.0, 0.0, "2026-06-19", 1000.0},
            {"IF2609", "IF", "hs300", "000300.XSHG", "沪深300", 0.0, 0.0, "2026-09-18", 900.0},
            {"IC2606", "IC", "zz500", "000905.XSHG", "中证500", 0.0, 0.0, "2026-06-19", 800.0},
            {"IM2606", "IM", "zz1000", "000852.XSHG", "中证1000", 0.0, 0.0, "2026-06-19", 700.0}
        };

        const auto instruments = basis_monitor::BuildXtpIndexInstruments(monitored_contracts);
        assert(instruments.size() == 3);
        assert(instruments[0] == "000300.XSHG");
        assert(instruments[1] == "000905.XSHG");
        assert(instruments[2] == "000852.XSHG");
    }

    {
        const std::vector<basis_monitor::MonitoredContract> monitored_contracts = {};
        const std::vector<std::string> fallback = {"000300", "000905"};

        const auto instruments = basis_monitor::BuildXtpIndexInstruments(monitored_contracts, fallback);
        assert(instruments.size() == 2);
        assert(instruments[0] == "000300");
        assert(instruments[1] == "000905");
    }

    {
        const std::vector<basis_monitor::MonitoredContract> monitored_contracts = {
            {"IF2606", "IF", "hs300", "", "沪深300", 0.0, 0.0, "2026-06-19", 1000.0},
            {"IC2606", "IC", "zz500", "000905.XSHG", "中证500", 0.0, 0.0, "2026-06-19", 800.0}
        };

        const auto instruments = basis_monitor::BuildXtpIndexInstruments(monitored_contracts);
        assert(instruments.size() == 1);
        assert(instruments[0] == "000905.XSHG");
    }

    return 0;
}
