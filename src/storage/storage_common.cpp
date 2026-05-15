#include "storage_common.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace basis_monitor::storage
{
namespace
{

std::tm ToLocalTm(std::time_t time_value)
{
    std::tm local_tm = {};
#ifdef _WIN32
    localtime_s(&local_tm, &time_value);
#else
    localtime_r(&time_value, &local_tm);
#endif
    return local_tm;
}

bool NeedsHeader(const std::filesystem::path& file_path)
{
    std::error_code error;
    const auto size = std::filesystem::exists(file_path, error) ? std::filesystem::file_size(file_path, error) : 0;
    return error || size == 0;
}

} // namespace

bool OpenCsvAppendFile(const std::filesystem::path& file_path, std::ofstream& output, bool& write_header)
{
    std::error_code error;
    if (!file_path.parent_path().empty())
    {
        std::filesystem::create_directories(file_path.parent_path(), error);
        if (error)
        {
            return false;
        }
    }

    write_header = NeedsHeader(file_path);
    output.open(file_path, std::ios::app | std::ios::binary);
    return output.is_open();
}

bool WriteCsvHeaderIfNeeded(std::ofstream& output, bool write_header, std::string_view header_line)
{
    if (!write_header)
    {
        return static_cast<bool>(output);
    }

    output << header_line << '\n';
    return static_cast<bool>(output);
}

std::string CurrentTimestamp()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto time_value = system_clock::to_time_t(now);
    const auto millis = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    const std::tm local_tm = ToLocalTm(time_value);

    std::ostringstream stream;
    stream << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    stream << '.' << std::setw(3) << std::setfill('0') << millis.count();
    return stream.str();
}

std::string EscapeCsvField(const std::string& value)
{
    const bool needs_quotes = value.find_first_of("\",\n\r") != std::string::npos;
    if (!needs_quotes)
    {
        return value;
    }

    std::string escaped;
    escaped.reserve(value.size() + 2);
    escaped.push_back('"');
    for (const char ch : value)
    {
        if (ch == '"')
        {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

} // namespace basis_monitor::storage
