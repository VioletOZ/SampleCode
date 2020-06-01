#pragma once

#include "StressBotLib/Scenario/ScenarioWithPacketHelper.h"
#include "StressBotLib/Scenario/ScenarioTaskHelper.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// '메일 기능 검증' 시나리오
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ScenarioMail : public ScenarioWithPacketHelper<ScenarioMail>
{
	friend class BotAgent;

private:
	static constexpr int MAIL_LIST_LIMIT_COUNT = 20;

	enum TaskIdx : uint16
	{
		CS_CHEAT_MAIL_LIST,
		//CS_REQ_MAIL_LIST_ACCOUNT,
		//CS_REQ_MAIL_LIST_USER,
		CS_REQ_MAIL_SEND,		
		CS_REQ_MAIL_READ,
		COMPLETE_SCENARIO
	};

	using MailTaskHelper = ScenarioTaskHelper<ScenarioMail, TaskIdx>;
	MailTaskHelper	mTaskHelper;


public:
	ScenarioMail();


public:
	void Init() override;
	void ProcessTask() override;
	uint16 GetCurrentTaskId() override { return mTaskHelper.GetTaskIdx(); }


public:
	// TaskHandlers
	void TaskHandler_CS_CHEAT_MAIL_LIST(AuthSessionContext* authSessionContext, GatewaySessionContext* gatewaySessionContext);
	void TaskHandler_CS_REQ_MAIL_LIST_ACCOUNT(AuthSessionContext* authSessionContext, GatewaySessionContext* gatewaySessionContext);
	void TaskHandler_CS_REQ_MAIL_LIST_USER(AuthSessionContext* authSessionContext, GatewaySessionContext* gatewaySessionContext);
	void TaskHandler_CS_REQ_MAIL_SEND(AuthSessionContext* authSessionContext, GatewaySessionContext* gatewaySessionContext);	
	void TaskHandler_CS_REQ_MAIL_READ(AuthSessionContext* authSessionContext, GatewaySessionContext* gatewaySessionContext);
	void TaskHandler_COMPLETE_SCENARIO(AuthSessionContext* authSessionContext, GatewaySessionContext* gatewaySessionContext);

	// PacketHandlers
	void PacketHandler_SC_ACK_MAIL_LIST(const uchar* buf, int len, AuthSessionContext* authSessionContext, GatewaySessionContext* gatewaySessionContext);
	void PacketHandler_SC_ACK_MAIL_SEND(const uchar* buf, int len, AuthSessionContext* authSessionContext, GatewaySessionContext* gatewaySessionContext);
	void PacketHandler_SC_ACK_MAIL_READ(const uchar* buf, int len, AuthSessionContext* authSessionContext, GatewaySessionContext* gatewaySessionContext);
};
