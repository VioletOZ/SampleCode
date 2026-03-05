#pragma once
#include "stdafx.h"
#include "Common.h"


class IMessageHandler
{
public:
    virtual ~IMessageHandler() = default;
    virtual void Handle(Session& session, const Message& msg) = 0;
};