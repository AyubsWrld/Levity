#include "notifier.hpp"
#include <curl/curl.h>

namespace ts {

bool NotificationStrategy::sendHttpPost(const std::string& url,
                                         const std::string& payload,
                                         const struct curl_slist* extra_headers) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    const struct curl_slist* current = extra_headers;
    while (current) {
        headers = curl_slist_append(headers, current->data);
        current = current->next;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK);
}


SlackStrategy::SlackStrategy(std::string url)
    : webhook_url(std::move(url)) {}

bool SlackStrategy::send(const std::string& message) {
    std::cout << "STUB INVOKED: " << __func__ << std::endl;
    std::cout << message << '\n';
    return false;
    //std::string payload = "{\"text\": \"" + message + "\"}";
    //return sendHttpPost(webhook_url, payload);
}


SendGridEmailStrategy::SendGridEmailStrategy(std::string key, std::string from, std::string to)
    : api_key(std::move(key)), from_email(std::move(from)), to_email(std::move(to)) {}

bool SendGridEmailStrategy::send(const std::string& message) {
    std::string url = "https://sendgrid.com";

    struct curl_slist* auth_header = nullptr;
    std::string auth_str = "Authorization: Bearer " + api_key;
    auth_header = curl_slist_append(auth_header, auth_str.c_str());

    std::string payload = "{"
        "\"personalizations\": [{\"to\": [{\"email\": \"" + to_email + "\"}]}],"
        "\"from\": {\"email\": \"" + from_email + "\"},"
        "\"subject\": \"Stakeholder Alert Notification\","
        "\"content\": [{\"type\": \"text/plain\", \"value\": \"" + message + "\"}]"
    "}";

    bool success = sendHttpPost(url, payload, auth_header);
    curl_slist_free_all(auth_header);
    return success;
}


void StakeholderNotifier::addChannel(std::unique_ptr<NotificationStrategy> strategy) {
    strategies.push_back(std::move(strategy));
}

void StakeholderNotifier::notifyAll(const std::string& message) {
    for (const auto& strategy : strategies) {
        strategy->send(message);
    }
}

namespace notifier {
    auto Run() -> int {
        curl_global_init(CURL_GLOBAL_DEFAULT);

        StakeholderNotifier notifier;

        notifier.addChannel(std::make_unique<SlackStrategy>(
            "https://slack.com"
        ));

        notifier.addChannel(std::make_unique<SendGridEmailStrategy>(
            "SG.your_sendgrid_api_key_here",
            "alerts@yourcompany.com",
            "stakeholder@yourcompany.com"
        ));

        std::cout << "Dispatching stakeholder updates..." << std::endl;
        // notifier.notifyAll("Critical System Update: Deploying patch version 2.4.1.");

        curl_global_cleanup();
        return 0;
    }
}
} // namespace ts