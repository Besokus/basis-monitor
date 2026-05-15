#include "basis_monitor/notify/wecom_robot_notifier.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <system_error>

namespace basis_monitor
{

namespace
{

std::string EscapeJson(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const char ch : value)
    {
        switch (ch)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

std::string ReadFileToString(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open())
    {
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string QuoteShellArgument(const std::filesystem::path& path)
{
    std::string text = path.string();
    std::string escaped;
    escaped.reserve(text.size() + 2);
    for (const char ch : text)
    {
        if (ch == '"')
        {
            escaped += "\\\"";
        }
        else
        {
            escaped.push_back(ch);
        }
    }
    return "\"" + escaped + "\"";
}

} // namespace

WeComRobotNotifier::WeComRobotNotifier(std::string webhook, Sender sender)
    : webhook_(std::move(webhook)),
      sender_(std::move(sender))
{
    if (!sender_)
    {
        sender_ = DefaultSender;
    }
}

NotificationResult WeComRobotNotifier::SendText(const std::string& message)
{
    if (webhook_.empty())
    {
        return NotificationResult{false, "disabled"};
    }

    return sender_(webhook_, BuildTextPayload(message));
}

NotificationResult WeComRobotNotifier::SendMarkdown(const std::string& message)
{
    if (webhook_.empty())
    {
        return NotificationResult{false, "disabled"};
    }

    return sender_(webhook_, BuildMarkdownPayload(message));
}

NotificationResult WeComRobotNotifier::SendImage(const std::filesystem::path& image_path)
{
    if (webhook_.empty())
    {
        return NotificationResult{false, "disabled"};
    }

    std::string payload;
    const auto payload_result = BuildImagePayloadFromFile(image_path, &payload);
    if (!payload_result.sent)
    {
        return payload_result;
    }

    return sender_(webhook_, payload);
}

std::string WeComRobotNotifier::BuildTextPayload(const std::string& message)
{
    std::ostringstream payload;
    payload << "{\"msgtype\":\"text\",\"text\":{\"content\":\""
            << EscapeJson(message)
            << "\"}}";
    return payload.str();
}

std::string WeComRobotNotifier::BuildMarkdownPayload(const std::string& message)
{
    std::ostringstream payload;
    payload << "{\"msgtype\":\"markdown\",\"markdown\":{\"content\":\""
            << EscapeJson(message)
            << "\"}}";
    return payload.str();
}

std::string WeComRobotNotifier::BuildImagePayload(const std::string& base64_content,
                                                  const std::string& md5_hex)
{
    std::ostringstream payload;
    payload << "{\"msgtype\":\"image\",\"image\":{\"base64\":\""
            << EscapeJson(base64_content)
            << "\",\"md5\":\""
            << EscapeJson(md5_hex)
            << "\"}}";
    return payload.str();
}

NotificationResult WeComRobotNotifier::ParseResponseBody(const std::string& response_body)
{
    static const std::regex errcode_pattern(R"json("errcode"\s*:\s*(-?\d+))json");
    static const std::regex errmsg_pattern(R"json("errmsg"\s*:\s*"([^"]*)")json");

    std::smatch errcode_match;
    if (!std::regex_search(response_body, errcode_match, errcode_pattern))
    {
        return NotificationResult{false, "http_response_invalid"};
    }

    const int errcode = std::stoi(errcode_match[1].str());

    std::smatch errmsg_match;
    std::string errmsg = "unknown";
    if (std::regex_search(response_body, errmsg_match, errmsg_pattern))
    {
        errmsg = errmsg_match[1].str();
    }

    if (errcode == 0)
    {
        return NotificationResult{true, "ok"};
    }

    return NotificationResult{
        false,
        "wecom_errcode_" + std::to_string(errcode) + ":" + errmsg
    };
}

NotificationResult WeComRobotNotifier::BuildImagePayloadFromFile(const std::filesystem::path& image_path,
                                                                 std::string* payload)
{
    if (payload == nullptr)
    {
        return NotificationResult{false, "payload_target_missing"};
    }
    if (!std::filesystem::exists(image_path))
    {
        return NotificationResult{false, "image_missing"};
    }

#ifdef _WIN32
    (void)image_path;
    return NotificationResult{false, "unsupported_on_windows"};
#else
    const auto script_path = std::filesystem::path("scripts") / "build_wecom_image_payload.py";
    if (!std::filesystem::exists(script_path))
    {
        return NotificationResult{false, "image_payload_script_missing"};
    }

    const auto payload_path = std::filesystem::temp_directory_path() /
        ("basis_monitor_wecom_image_payload_" + std::to_string(static_cast<long long>(std::rand())) + ".json");

    const std::string command =
        "python3 " + QuoteShellArgument(script_path) + " " +
        QuoteShellArgument(image_path) + " " +
        QuoteShellArgument(payload_path);
    const int exit_code = std::system(command.c_str());
    if (exit_code != 0)
    {
        std::error_code ignored;
        std::filesystem::remove(payload_path, ignored);
        return NotificationResult{false, "image_payload_build_failed_" + std::to_string(exit_code)};
    }

    *payload = ReadFileToString(payload_path);
    std::error_code ignored;
    std::filesystem::remove(payload_path, ignored);

    if (payload->empty())
    {
        return NotificationResult{false, "image_payload_empty"};
    }

    return NotificationResult{true, "ok"};
#endif
}

NotificationResult WeComRobotNotifier::DefaultSender(const std::string& webhook, const std::string& payload)
{
#ifdef _WIN32
    (void)webhook;
    (void)payload;
    return NotificationResult{false, "unsupported_on_windows"};
#else
    const auto temp_path = std::filesystem::temp_directory_path() /
        ("basis_monitor_wecom_payload_" + std::to_string(static_cast<long long>(std::rand())) + ".json");
    const auto response_path = std::filesystem::temp_directory_path() /
        ("basis_monitor_wecom_response_" + std::to_string(static_cast<long long>(std::rand())) + ".json");

    {
        std::ofstream output(temp_path, std::ios::binary | std::ios::trunc);
        if (!output.is_open())
        {
            return NotificationResult{false, "payload_open_failed"};
        }
        output << payload;
    }

    const std::string command =
        "curl -sS --connect-timeout 3 --max-time 8 -H \"Content-Type: application/json\" -X POST --data-binary @\"" +
        temp_path.string() + "\" \"" + webhook + "\" -o \"" + response_path.string() + "\"";
    const int exit_code = std::system(command.c_str());

    const std::string response_body = ReadFileToString(response_path);

    std::error_code ignored;
    std::filesystem::remove(temp_path, ignored);
    std::filesystem::remove(response_path, ignored);

    if (exit_code != 0)
    {
        return NotificationResult{false, "curl_failed_" + std::to_string(exit_code)};
    }

    return ParseResponseBody(response_body);
#endif
}

} // namespace basis_monitor
