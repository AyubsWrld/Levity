#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Forward declaration of libcurl types to avoid leaking curl headers to consumers
typedef void CURL;
struct curl_slist;

namespace ts {

    class NotificationStrategy {
    public:
        virtual ~NotificationStrategy() = default;
        virtual bool send(const std::string& message) = 0;

    protected:
        bool sendHttpPost(const std::string& url,
                          const std::string& payload,
                          const struct curl_slist* extra_headers = nullptr);
    };

    class SlackStrategy : public NotificationStrategy {
    private:
        std::string webhook_url;

    public:
        explicit SlackStrategy(std::string url);
        bool send(const std::string& message) override;
    };

    class SendGridEmailStrategy : public NotificationStrategy {
    private:
        std::string api_key;
        std::string from_email;
        std::string to_email;

    public:
        SendGridEmailStrategy(std::string key, std::string from, std::string to);
        bool send(const std::string& message) override;
    };

    class StakeholderNotifier {
    private:
        std::vector<std::unique_ptr<NotificationStrategy>> strategies;

    public:
        void addChannel(std::unique_ptr<NotificationStrategy> strategy);
        void notifyAll(const std::string& message);
    };

namespace notifier {
    auto Run() -> int;
} // namespace notifier

} // namespace ts