#include <cassert>
#include <cmath>

#include "basis_monitor/monitor/basis_calculator.h"

int main()
{
    using basis_monitor::CalculateAnnualizedBasis;

    {
        const auto result = CalculateAnnualizedBasis(6300.0, 6190.0, 23);
        assert(result.valid);
        assert(result.remaining_days == 23);
        assert(std::abs(result.basis - 110.0) < 0.001);
        assert(std::abs(result.annual_rate - 27.71) < 0.02);
    }

    {
        const auto result = CalculateAnnualizedBasis(6300.0, 6400.0, 23);
        assert(result.valid);
        assert(result.basis == -100.0);
        assert(result.annual_rate < 0.0);
    }

    {
        const auto result = CalculateAnnualizedBasis(6300.0, 6230.0, 0);
        assert(!result.valid);
        assert(result.remaining_days == 0);
    }

    {
        const auto result = CalculateAnnualizedBasis(0.0, 6230.0, 10);
        assert(!result.valid);
        assert(result.remaining_days == 10);
    }

    return 0;
}
