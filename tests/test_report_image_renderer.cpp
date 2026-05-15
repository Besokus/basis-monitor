#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "basis_monitor/report/report_image_formatter.h"
#include "basis_monitor/report/report_image_renderer.h"

namespace
{

struct TempDirCleanup
{
    std::filesystem::path path;

    ~TempDirCleanup()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

std::string ReadHeader(const std::filesystem::path& path, std::size_t size)
{
    std::ifstream input(path, std::ios::binary);
    assert(input.is_open());
    std::string header(size, '\0');
    input.read(&header[0], static_cast<std::streamsize>(size));
    header.resize(static_cast<std::size_t>(input.gcount()));
    return header;
}

} // namespace

int main()
{
    basis_monitor::BasisReportImageDocument document = {};
    document.title = "BASIS MONITOR 11:30 REPORT";
    document.subtitle = "DATE 2026-04-01";
    document.columns = {"CONTRACT", "PRICE", "CHG", "CHG%", "BASIS", "ANNUAL", "DTE", "WARNING"};

    basis_monitor::BasisReportImageGroup group = {};
    group.name = "IH";
    group.rows.push_back(basis_monitor::BasisReportImageRow{
        "IH2604",
        16,
        "2818.0",
        "-2.0",
        "-0.07%",
        "8.25",
        "-0.80%",
        "16",
        "-0.80%",
        true});
    document.groups.push_back(group);

    const auto source_root = std::filesystem::path(__FILE__).parent_path().parent_path();
    const auto script_path = source_root / "scripts" / "render_basis_report_image.py";
    const auto temp_root = std::filesystem::temp_directory_path() / "basis_monitor_report_image_renderer_test";
    TempDirCleanup cleanup{temp_root};
    std::filesystem::remove_all(temp_root);
    std::filesystem::create_directories(temp_root);

    basis_monitor::ReportImageRenderer renderer(script_path);
    const auto output_path = temp_root / "report.png";
    const auto result = renderer.Render(document, output_path);

    assert(result.success);
    assert(result.reason == "ok");
    assert(std::filesystem::exists(output_path));

    const auto header = ReadHeader(output_path, 8);
    assert(header.size() == 8);
    assert(static_cast<unsigned char>(header[0]) == 0x89);
    assert(header[1] == 'P');
    assert(header[2] == 'N');
    assert(header[3] == 'G');

    return 0;
}
