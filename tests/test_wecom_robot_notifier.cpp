#include <cassert>
#include <string>

#include "basis_monitor/notify/wecom_robot_notifier.h"

int main()
{
    basis_monitor::WeComRobotNotifier disabled_notifier("");
    const auto disabled_result = disabled_notifier.SendText("test message");
    assert(!disabled_result.sent);
    assert(disabled_result.reason == "disabled");

    const auto payload = basis_monitor::WeComRobotNotifier::BuildTextPayload("line1\nline2");
    assert(payload.find("\"msgtype\":\"text\"") != std::string::npos);
    assert(payload.find("\"content\":\"line1\\nline2\"") != std::string::npos);

    const auto image_payload = basis_monitor::WeComRobotNotifier::BuildImagePayload("QUJD", "902fbdd2b1df0c4f70b4a5d23525e932");
    assert(image_payload.find("\"msgtype\":\"image\"") != std::string::npos);
    assert(image_payload.find("\"base64\":\"QUJD\"") != std::string::npos);
    assert(image_payload.find("\"md5\":\"902fbdd2b1df0c4f70b4a5d23525e932\"") != std::string::npos);

    const auto markdown_payload = basis_monitor::WeComRobotNotifier::BuildMarkdownPayload("### Title\n>Detail");
    assert(markdown_payload.find("\"msgtype\":\"markdown\"") != std::string::npos);
    assert(markdown_payload.find("\"content\":\"### Title\\n>Detail\"") != std::string::npos);

    const auto success_response = basis_monitor::WeComRobotNotifier::ParseResponseBody("{\"errcode\":0,\"errmsg\":\"ok\"}");
    assert(success_response.sent);
    assert(success_response.reason == "ok");

    const auto failure_response = basis_monitor::WeComRobotNotifier::ParseResponseBody("{\"errcode\":93000,\"errmsg\":\"invalid webhook key\"}");
    assert(!failure_response.sent);
    assert(failure_response.reason == "wecom_errcode_93000:invalid webhook key");

    const auto invalid_response = basis_monitor::WeComRobotNotifier::ParseResponseBody("{\"errmsg\":\"missing errcode\"}");
    assert(!invalid_response.sent);
    assert(invalid_response.reason == "http_response_invalid");

    bool sender_called = false;
    basis_monitor::WeComRobotNotifier stubbed_notifier(
        "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=test-key",
        [&sender_called](const std::string& webhook, const std::string& message_payload) {
            sender_called = true;
            assert(webhook == "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=test-key");
            assert(message_payload.find("\"msgtype\":\"text\"") != std::string::npos);
            return basis_monitor::NotificationResult{true, "ok"};
        });

    const auto sent_result = stubbed_notifier.SendText("negative annual basis alert");
    assert(sender_called);
    assert(sent_result.sent);
    assert(sent_result.reason == "ok");

    bool markdown_sender_called = false;
    basis_monitor::WeComRobotNotifier markdown_notifier(
        "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=test-key",
        [&markdown_sender_called](const std::string&, const std::string& message_payload) {
            markdown_sender_called = true;
            assert(message_payload.find("\"msgtype\":\"markdown\"") != std::string::npos);
            return basis_monitor::NotificationResult{true, "ok"};
        });
    const auto markdown_result = markdown_notifier.SendMarkdown("### Annual Basis Alert");
    assert(markdown_sender_called);
    assert(markdown_result.sent);
    assert(markdown_result.reason == "ok");

    basis_monitor::WeComRobotNotifier failing_notifier(
        "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=test-key",
        [](const std::string&, const std::string&) {
            return basis_monitor::NotificationResult{false, "wecom_errcode_93000:invalid webhook key"};
        });
    const auto failed_result = failing_notifier.SendText("negative annual basis alert");
    assert(!failed_result.sent);
    assert(failed_result.reason == "wecom_errcode_93000:invalid webhook key");

    return 0;
}
