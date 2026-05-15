#pragma once

#include <filesystem>
#include <string>

#include "basis_monitor/report/report_image_formatter.h"

namespace basis_monitor
{

struct ReportImageRenderResult
{
    bool success = false;
    std::filesystem::path image_path;
    std::string reason;
};

class ReportImageRenderer
{
public:
    explicit ReportImageRenderer(std::filesystem::path script_path = std::filesystem::path("scripts") / "render_basis_report_image.py");

    ReportImageRenderResult Render(const BasisReportImageDocument& document,
                                   const std::filesystem::path& output_path) const;

private:
    std::filesystem::path script_path_;
};

} // namespace basis_monitor
