#include "basis_monitor/report/report_image_renderer.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace basis_monitor
{

namespace
{

std::string QuoteShellArgument(const std::filesystem::path& path)
{
    std::string text = path.string();
    std::string escaped;
    escaped.reserve(text.size() + 2);
    for (const char ch : text)
    {
        if (ch == '"')
        {
            escaped += "\\\"";
        }
        else
        {
            escaped.push_back(ch);
        }
    }
    return "\"" + escaped + "\"";
}

} // namespace

ReportImageRenderer::ReportImageRenderer(std::filesystem::path script_path)
    : script_path_(std::move(script_path))
{
}

ReportImageRenderResult ReportImageRenderer::Render(const BasisReportImageDocument& document,
                                                    const std::filesystem::path& output_path) const
{
#ifdef _WIN32
    (void)document;
    (void)output_path;
    return ReportImageRenderResult{false, {}, "unsupported_on_windows"};
#else
    if (!std::filesystem::exists(script_path_))
    {
        return ReportImageRenderResult{false, {}, "renderer_script_missing"};
    }

    std::filesystem::create_directories(output_path.parent_path());

    const auto temp_input_path = std::filesystem::temp_directory_path() /
        ("basis_monitor_report_image_" + std::to_string(static_cast<long long>(std::rand())) + ".json");
    {
        std::ofstream output(temp_input_path, std::ios::binary | std::ios::trunc);
        if (!output.is_open())
        {
            return ReportImageRenderResult{false, {}, "renderer_input_open_failed"};
        }
        output << SerializeBasisReportImageDocument(document);
    }

    const std::string command =
        "python3 " + QuoteShellArgument(script_path_) + " " +
        QuoteShellArgument(temp_input_path) + " " +
        QuoteShellArgument(output_path);
    const int exit_code = std::system(command.c_str());

    std::error_code ignored;
    std::filesystem::remove(temp_input_path, ignored);

    if (exit_code != 0)
    {
        return ReportImageRenderResult{false, {}, "renderer_failed_" + std::to_string(exit_code)};
    }
    if (!std::filesystem::exists(output_path))
    {
        return ReportImageRenderResult{false, {}, "renderer_output_missing"};
    }

    return ReportImageRenderResult{true, output_path, "ok"};
#endif
}

} // namespace basis_monitor
