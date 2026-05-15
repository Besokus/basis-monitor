#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#include "basis_monitor/storage/report_store.h"

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

std::string ReadWholeFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    assert(input.is_open());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

} // namespace

int main()
{
    const auto temp_root = std::filesystem::temp_directory_path() / "basis_monitor_report_store_test";
    TempDirCleanup cleanup{temp_root};
    std::filesystem::remove_all(temp_root);

    basis_monitor::ReportStore store(temp_root);

    const std::string midday_report = "[Basis Monitor] 11:30 最新基差表\n[GROUP] IC\nIC2606 ...\n";
    const auto midday_path = store.Write("2026-03-31", basis_monitor::BasisReportMoment::Midday1130, midday_report);
    assert(midday_path.filename().string() == "2026-03-31_1130_latest_basis.txt");
    assert(std::filesystem::exists(midday_path));
    assert(ReadWholeFile(midday_path) == midday_report);

    const std::string close_report = "[Basis Monitor] 15:00 最新基差表\n[GROUP] IF\nIF2606 ...\n";
    const auto close_path = store.Write("2026-03-31", basis_monitor::BasisReportMoment::Close1500, close_report);
    assert(close_path.filename().string() == "2026-03-31_1500_latest_basis.txt");
    assert(std::filesystem::exists(close_path));
    assert(ReadWholeFile(close_path) == close_report);

    const auto midday_image_path = store.ImagePath("2026-03-31", basis_monitor::BasisReportMoment::Midday1130);
    assert(midday_image_path.filename().string() == "2026-03-31_1130_latest_basis.png");

    const auto close_image_path = store.ImagePath("2026-03-31", basis_monitor::BasisReportMoment::Close1500);
    assert(close_image_path.filename().string() == "2026-03-31_1500_latest_basis.png");

    return 0;
}
