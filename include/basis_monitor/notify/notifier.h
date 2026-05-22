#pragma once

#include <filesystem>
#include <string>

namespace basis_monitor
{

struct NotificationResult
{
    bool sent = false;
    std::string reason;
};

class Notifier
{
public:
    virtual ~Notifier() = default;

    virtual NotificationResult SendText(const std::string& message) = 0;
    virtual NotificationResult SendMarkdown(const std::string& message) = 0;
    virtual NotificationResult SendImage(const std::filesystem::path& image_path) = 0;
};

} // namespace basis_monitor
