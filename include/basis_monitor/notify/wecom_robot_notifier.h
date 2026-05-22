#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "basis_monitor/notify/notifier.h"

namespace basis_monitor
{

class WeComRobotNotifier final : public Notifier
{
public:
    using Sender = std::function<NotificationResult(const std::string&, const std::string&)>;

    explicit WeComRobotNotifier(std::string webhook, Sender sender = {});

    NotificationResult SendText(const std::string& message) override;
    NotificationResult SendMarkdown(const std::string& message) override;
    NotificationResult SendImage(const std::filesystem::path& image_path) override;

    static std::string BuildTextPayload(const std::string& message);
    static std::string BuildMarkdownPayload(const std::string& message);
    static std::string BuildImagePayload(const std::string& base64_content,
                                         const std::string& md5_hex);
    static NotificationResult ParseResponseBody(const std::string& response_body);

private:
    static NotificationResult DefaultSender(const std::string& webhook, const std::string& payload);
    static NotificationResult BuildImagePayloadFromFile(const std::filesystem::path& image_path,
                                                        std::string* payload);

    std::string webhook_;
    Sender sender_;
};

} // namespace basis_monitor
