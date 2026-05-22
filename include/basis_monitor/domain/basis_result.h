#pragma once

namespace basis_monitor
{

struct BasisResult
{
    bool valid = false;
    double basis = 0.0;
    double annual_rate = 0.0;
    int remaining_days = 0;
};

} // namespace basis_monitor
