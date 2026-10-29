
#pragma once

#include <string>
#include <mutex>
#include <thread>
#include <deque>
#include <cstdint>

#include <nlohmann/json.hpp>

enum class AccountLoginMethod
{
    Standard,
    Morda
};

enum class AccountLoginStatus
{
    Idle,
    InProgress,
    Success,
    Failed
};

struct AccountLoginResult
{
    AccountLoginStatus status = AccountLoginStatus::Idle;
    nlohmann::json data;
    std::string message;
};

class AccountClient
{
public:
    AccountClient() = default;
    ~AccountClient();

    AccountClient(const AccountClient&) = delete;
    AccountClient& operator=(const AccountClient&) = delete;

    // Вызывать из основного потока.
    bool login(
        const std::string& donateId,
        const std::string& password,
        AccountLoginMethod method = AccountLoginMethod::Standard
    );

    // Вызывать из основного потока регулярно.
    void update();

    AccountLoginStatus status() const;
    AccountLoginResult lastResult() const;

private:
    struct HttpResult
    {
        unsigned long httpStatus = 0;
        std::string body;
        std::string error;
    };

    void runLogin(
        std::string donateId,
        std::string password,
        AccountLoginMethod method
    );

    HttpResult postLogin(
        const std::string& url,
        const std::string& donateId,
        const std::string& password
    );

    void publishResult(AccountLoginResult result);

    mutable std::mutex m_mutex;
    std::thread m_worker;
    std::deque<AccountLoginResult> m_results;

    AccountLoginStatus m_status = AccountLoginStatus::Idle;
    AccountLoginResult m_lastResult;

    bool m_busy = false;
};
