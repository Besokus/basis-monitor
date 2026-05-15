#pragma once

#include <string>

namespace basis_monitor
{

void InitializeLogger(const std::string& runtime_log_path, const std::string& alert_log_path);
void InitializeLogger(const std::string& log_path);
void ShutdownLogger();
void Log(const char* format, ...);
void LogAlert(const char* format, ...);

} // namespace basis_monitor
