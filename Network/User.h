#pragma once
#include "stdafx.h"
#include "Message.h"

#include "Card.h"

class User : public IMessageHandler
{
public:
    User() = default;
    User(uint32_t user_id, std::string user_name) : _session_id(""), _user_id(0), _user_name(""), _card(nullptr)
    {
        std::chrono::sys_days today_sd = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
        _update_date = today_sd;
    };

    void Handle(Session& session, const Message& msg) override {};

    uint32_t GetId() const { return _user_id; }

    std::string GetName() const { return _user_name; }
    std::chrono::year_month_day GetDate() const { return _update_date; }

    Card* GetCard() const { return _card; }

private:
    std::string _session_id;
    uint32_t _user_id;
    std::string _user_name;
    std::chrono::year_month_day _update_date;

    Card* _card;
};

