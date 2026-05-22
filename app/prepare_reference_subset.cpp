#include <chrono>
#include <exception>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "basis_monitor/data/contract_selector.h"
#include "basis_monitor/data/reference_data_loader.h"
#include "basis_monitor/data/reference_subset_builder.h"

namespace
{

std::string CurrentTradingDate()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = {};
#ifdef _WIN32
    localtime_s(&local_time, &current_time);
#else
    localtime_r(&current_time, &local_time);
#endif

    std::ostringstream stream;
    stream << std::setfill('0')
           << std::setw(4) << (local_time.tm_year + 1900)
           << '-'
           << std::setw(2) << (local_time.tm_mon + 1)
           << '-'
           << std::setw(2) << local_time.tm_mday;
    return stream.str();
}

void PrintUsage(const char* program_name)
{
    std::cerr << "Usage: " << program_name
              << " <future_metadata_dir> <index_metadata_dir> <future_eod_dir> <index_eod_dir> <output_root>\n";
}

} // namespace

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 6)
        {
            PrintUsage(argc > 0 ? argv[0] : "prepare_reference_subset");
            return 1;
        }

        basis_monitor::ReferenceDataDirectories directories = {};
        directories.future_metadata_dir = std::filesystem::path(argv[1]);
        directories.index_metadata_dir = std::filesystem::path(argv[2]);
        directories.future_eod_dir = std::filesystem::path(argv[3]);
        directories.index_eod_dir = std::filesystem::path(argv[4]);
        const std::filesystem::path output_root = std::filesystem::path(argv[5]);

        const auto reference_data = basis_monitor::LoadReferenceData(directories);
        std::cerr << "[REFERENCE_SOURCE] future_metadata="
                  << reference_data.future_metadata_source_path
                  << " date=" << reference_data.future_metadata_date << '\n';
        std::cerr << "[REFERENCE_SOURCE] index_metadata="
                  << reference_data.index_metadata_source_path
                  << " date=" << reference_data.index_metadata_date << '\n';
        std::cerr << "[REFERENCE_SOURCE] future_eod="
                  << reference_data.future_eod_source_path
                  << " date=" << reference_data.future_eod_date << '\n';
        std::cerr << "[REFERENCE_SOURCE] index_eod="
                  << reference_data.index_eod_source_path
                  << " date=" << reference_data.index_eod_date << '\n';
        const auto selection = basis_monitor::SelectPerProductTop4(reference_data, CurrentTradingDate());
        const auto subset = basis_monitor::BuildReferenceDataSubset(reference_data, selection);
        basis_monitor::WriteReferenceDataSubset(subset, output_root);

        std::cerr << "Prepared reference subset under " << output_root.string() << '\n';
        for (const auto& warning : selection.warnings)
        {
            std::cerr << "[SELECTION_WARNING] contract=" << warning.instrument_id
                      << " reason=" << warning.reason << '\n';
        }

        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
