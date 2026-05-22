#pragma once

#include "basis_monitor/domain/basis_result.h"

namespace basis_monitor
{

BasisResult CalculateAnnualizedBasis(double spot_price, double future_price, int remaining_days);

} // namespace basis_monitor
