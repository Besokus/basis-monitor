#include "basis_monitor/logging/logger.h"

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

namespace basis_monitor
{

namespace
{

std::mutex g_log_mutex;
FILE* g_runtime_log_file = nullptr;
FILE* g_alert_log_file = nullptr;

FILE* OpenLogFile(const std::string& log_path)
{
    if (log_path.empty())
    {
        return nullptr;
    }
    return std::fopen(log_path.c_str(), "a");
}

void CloseLogFile(FILE*& file)
{
    if (file != nullptr)
    {
        std::fclose(file);
        file = nullptr;
    }
}

void WriteFormattedMessage(bool write_alert_log, const char* format, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    const int needed = std::vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    std::vector<char> buffer(static_cast<std::size_t>(needed >= 0 ? needed + 1 : 1024), '\0');
    std::vsnprintf(buffer.data(), buffer.size(), format, args);

    std::fputs(buffer.data(), stdout);
    std::fflush(stdout);

    if (g_runtime_log_file != nullptr)
    {
        std::fputs(buffer.data(), g_runtime_log_file);
        std::fflush(g_runtime_log_file);
    }

    if (write_alert_log && g_alert_log_file != nullptr)
    {
        std::fputs(buffer.data(), g_alert_log_file);
        std::fflush(g_alert_log_file);
    }
}

} // namespace

void InitializeLogger(const std::string& runtime_log_path, const std::string& alert_log_path)
{
    std::lock_guard<std::mutex> lock(g_log_mutex);
    if (g_runtime_log_file != nullptr)
    {
        return;
    }

    g_runtime_log_file = OpenLogFile(runtime_log_path);
    if (g_runtime_log_file == nullptr)
    {
        return;
    }

    g_alert_log_file = OpenLogFile(alert_log_path);
}

void InitializeLogger(const std::string& log_path)
{
    std::lock_guard<std::mutex> lock(g_log_mutex);
    if (g_runtime_log_file != nullptr)
    {
        return;
    }
    g_runtime_log_file = OpenLogFile(log_path);
}

void ShutdownLogger()
{
    std::lock_guard<std::mutex> lock(g_log_mutex);
    CloseLogFile(g_runtime_log_file);
    CloseLogFile(g_alert_log_file);
}

void Log(const char* format, ...)
{
    std::lock_guard<std::mutex> lock(g_log_mutex);

    va_list args;
    va_start(args, format);
    WriteFormattedMessage(false, format, args);
    va_end(args);
}

void LogAlert(const char* format, ...)
{
    std::lock_guard<std::mutex> lock(g_log_mutex);

    va_list args;
    va_start(args, format);
    WriteFormattedMessage(true, format, args);
    va_end(args);
}

} // namespace basis_monitor
