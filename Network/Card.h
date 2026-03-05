#pragma once
#include "stdafx.h"
#include "Message.h"

class Card : public IMessageHandler
{
public:
    void Handle(Session& session, const Message& msg) override {};

private:

};

