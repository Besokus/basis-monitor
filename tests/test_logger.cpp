#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "basis_monitor/logging/logger.h"

namespace
{

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    namespace fs = std::filesystem;

    const auto temp_dir = fs::temp_directory_path() / "basis_monitor_logger_test";
    fs::create_directories(temp_dir);

    const auto runtime_log = temp_dir / "runtime.log";
    const auto alert_log = temp_dir / "alert.log";
    const auto terminal_log = temp_dir / "terminal.log";

    const auto redirected_stdout = std::freopen(terminal_log.string().c_str(), "w", stdout);
    assert(redirected_stdout != nullptr);

    basis_monitor::InitializeLogger(runtime_log.string(), alert_log.string());
    basis_monitor::Log("[TEST] runtime message\n");
    basis_monitor::LogAlert("[TEST] alert message\n");
    basis_monitor::LogAlert("[TEST] recovery message\n");
    basis_monitor::ShutdownLogger();
    std::fflush(stdout);

    const auto runtime_contents = ReadFile(runtime_log);
    const auto alert_contents = ReadFile(alert_log);
    const auto terminal_contents = ReadFile(terminal_log);

    assert(runtime_contents.find("[TEST] runtime message") != std::string::npos);
    assert(runtime_contents.find("[TEST] alert message") != std::string::npos);
    assert(runtime_contents.find("[TEST] recovery message") != std::string::npos);

    assert(alert_contents.find("[TEST] runtime message") == std::string::npos);
    assert(alert_contents.find("[TEST] alert message") != std::string::npos);
    assert(alert_contents.find("[TEST] recovery message") != std::string::npos);

    assert(terminal_contents.find("[TEST] runtime message") != std::string::npos);
    assert(terminal_contents.find("[TEST] alert message") != std::string::npos);
    assert(terminal_contents.find("[TEST] recovery message") != std::string::npos);

    return 0;
}
