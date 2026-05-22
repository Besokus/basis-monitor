#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace basis_monitor::storage
{

bool OpenCsvAppendFile(const std::filesystem::path& file_path, std::ofstream& output, bool& write_header);
bool WriteCsvHeaderIfNeeded(std::ofstream& output, bool write_header, std::string_view header_line);
std::string CurrentTimestamp();
std::string EscapeCsvField(const std::string& value);

} // namespace basis_monitor::storage
