#include "basis_monitor/monitor/basis_calculator.h"

namespace basis_monitor
{

BasisResult CalculateAnnualizedBasis(double spot_price, double future_price, int remaining_days)
{
    BasisResult result = {};
    result.remaining_days = remaining_days;

    if (remaining_days <= 0 || spot_price <= 0.0)
    {
        return result;
    }

    result.valid = true;
    result.basis = spot_price - future_price;
    result.annual_rate = result.basis / spot_price * (365.0 / static_cast<double>(remaining_days)) * 100.0;
    return result;
}

} // namespace basis_monitor
