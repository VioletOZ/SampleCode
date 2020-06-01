#include "ScenarioMail.h"

#include "StressBotLib/Network/Session/AuthSession.h"
#include "StressBotLib/Network/Session/GatewaySession.h"
#include "StressBotLib/Network/Session/AuthSessionContext.h"
#include "StressBotLib/Network/Session/GatewaySessionContext.h"
#include "TechShared/PacketDefinition/GeneratedHeaders/PdlValidate.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ScenarioMail::ScenarioMail()
	:
	mTaskHelper(this, TaskIdx::COMPLETE_SCENARIO)
{
	TRACE;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::Init()
{
	TRACE;

	mTaskHelper.RegTaskHandler(TaskIdx::CS_CHEAT_MAIL_LIST, &ScenarioMail::TaskHandler_CS_CHEAT_MAIL_LIST);
	/* 이부분은 과거 우편 받기 테스트용 주석을 풀면 테스트 가능
	mTaskHelper.RegTaskHandler(TaskIdx::CS_REQ_MAIL_LIST_ACCOUNT, &ScenarioMail::TaskHandler_CS_REQ_MAIL_LIST_ACCOUNT);
	mTaskHelper.RegTaskHandler(TaskIdx::CS_REQ_MAIL_LIST_USER, &ScenarioMail::TaskHandler_CS_REQ_MAIL_LIST_USER);
	*/
	mTaskHelper.RegTaskHandler(TaskIdx::CS_REQ_MAIL_SEND, &ScenarioMail::TaskHandler_CS_REQ_MAIL_SEND);		
	mTaskHelper.RegTaskHandler(TaskIdx::CS_REQ_MAIL_READ, &ScenarioMail::TaskHandler_CS_REQ_MAIL_READ);
	mTaskHelper.RegTaskHandler(TaskIdx::COMPLETE_SCENARIO, &ScenarioMail::TaskHandler_COMPLETE_SCENARIO);

	RegPacketHandler(PD::SC::SC_ACK_MAIL_LIST, &ScenarioMail::PacketHandler_SC_ACK_MAIL_LIST);
	RegPacketHandler(PD::SC::SC_ACK_MAIL_SEND, &ScenarioMail::PacketHandler_SC_ACK_MAIL_SEND);
	RegPacketHandler(PD::SC::SC_ACK_MAIL_READ, &ScenarioMail::PacketHandler_SC_ACK_MAIL_READ);
	
	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::ProcessTask()
{
	TRACE;

	AuthSessionContext* authSessionContext = GetAuthSessionContext();
	GatewaySessionContext* gatewaySessionContext = GetGatewaySessionContext();
	if (false == mTaskHelper.ProcessTask(authSessionContext, gatewaySessionContext))
	{
		GConsolePrinter->OutConsoleAsync(Color::BLACK, L"false == mTaskHelper.HasTaskHandler()\n");
		_ASSERT_CRASH(false);
	}

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::TaskHandler_CS_CHEAT_MAIL_LIST(
	AuthSessionContext* authSessionContext,
	GatewaySessionContext* gatewaySessionContext
)
{
	TRACE;

	GConsolePrinter->OutConsoleAsync(Color::MAGENTA, L"ScenarioMail::TaskHandler_CS_CHEAT_MAIL_LIST()\n");

	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != authSessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != gatewaySessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		authSessionContext->mAuthSessionState == AuthSessionContext::AUTH_SESSION_STATE::CLOSED
	);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		gatewaySessionContext->mGatewaySessionState == GatewaySessionContext::GATEWAY_SESSION_STATE::LOGINED
	);

	GatewaySession* gatewaySession = GetGatewaySession();	

	// 우편목록 갱신 0 = ACCOUNT, 1 = USER
	gatewaySession->Send_CG_ADMIN_COMMAND_QA(authSessionContext->mAccountId, L"refreshMail 1"); 
	
	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::TaskHandler_CS_REQ_MAIL_LIST_ACCOUNT(
	AuthSessionContext* authSessionContext,
	GatewaySessionContext* gatewaySessionContext
)
{
	TRACE;

	GConsolePrinter->OutConsoleAsync(Color::MAGENTA, L"ScenarioMail::TaskHandler_CS_REQ_MAIL_LIST_ACCOUNT()\n");

	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != authSessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != gatewaySessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		authSessionContext->mAuthSessionState == AuthSessionContext::AUTH_SESSION_STATE::CLOSED
	);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		gatewaySessionContext->mGatewaySessionState == GatewaySessionContext::GATEWAY_SESSION_STATE::LOGINED
	);

	GatewaySession* gatewaySession = GetGatewaySession();
	gatewaySession->Send_CG_REQ_MAIL_LIST(
		authSessionContext->mAccountId, 
		MailBoxType::ACCOUNT, 
		gatewaySessionContext->GetOldestAccountMailDBId(),
		MAIL_LIST_LIMIT_COUNT
	);

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::TaskHandler_CS_REQ_MAIL_LIST_USER(
	AuthSessionContext* authSessionContext,
	GatewaySessionContext* gatewaySessionContext
)
{
	TRACE;

	GConsolePrinter->OutConsoleAsync(Color::MAGENTA, L"ScenarioMail::TaskHandler_CS_REQ_MAIL_LIST_USER()\n");

	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != authSessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != gatewaySessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		authSessionContext->mAuthSessionState == AuthSessionContext::AUTH_SESSION_STATE::CLOSED
	);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		gatewaySessionContext->mGatewaySessionState == GatewaySessionContext::GATEWAY_SESSION_STATE::LOGINED
	);	

	GatewaySession* gatewaySession = GetGatewaySession();
	gatewaySession->Send_CG_REQ_MAIL_LIST(
		authSessionContext->mAccountId,
		MailBoxType::USER,
		gatewaySessionContext->GetOldestUserMailDBId(),
		MAIL_LIST_LIMIT_COUNT
	);

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::TaskHandler_CS_REQ_MAIL_SEND(
	AuthSessionContext* authSessionContext,
	GatewaySessionContext* gatewaySessionContext
)
{
	TRACE;

	GConsolePrinter->OutConsoleAsync(Color::MAGENTA, L"ScenarioMail::TaskHandler_CS_REQ_MAIL_SEND()\n");

	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != authSessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != gatewaySessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		authSessionContext->mAuthSessionState == AuthSessionContext::AUTH_SESSION_STATE::CLOSED
	);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		gatewaySessionContext->mGatewaySessionState == GatewaySessionContext::GATEWAY_SESSION_STATE::LOGINED
	);

	// 테스트용으로 일단 계정메일함으로 보내보자. 나에게 보내는 테스트.
	{
		GatewaySession* gatewaySession = GetGatewaySession();
		gatewaySession->Send_CG_REQ_MAIL_SEND(authSessionContext->mAccountId, MailBoxType::ACCOUNT);
	}	

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::TaskHandler_CS_REQ_MAIL_READ(
	AuthSessionContext* authSessionContext,
	GatewaySessionContext* gatewaySessionContext
)
{
	TRACE;

	GConsolePrinter->OutConsoleAsync(Color::MAGENTA, L"ScenarioMail::TaskHandler_CS_REQ_MAIL_READ()\n");

	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != authSessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != gatewaySessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		authSessionContext->mAuthSessionState == AuthSessionContext::AUTH_SESSION_STATE::CLOSED
	);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(
		gatewaySessionContext->mGatewaySessionState == GatewaySessionContext::GATEWAY_SESSION_STATE::LOGINED
	);
		
	// 실제로는 클라가 받으려는 MailDBId를 모아서 받기 요청하도록 할 예정입니다
	// 하지만 스트레스봇 테스트는 그냥 자신이 가진 메일을 전부 받기 시도하는 것으로 구현합니다(최대 20개)
	int count = 0;
	MailDBIdList mailDBIds;
	for (const auto& it : gatewaySessionContext->mMailBoxLists[static_cast<uint16>(MailBoxType::ACCOUNT)])
	{
		mailDBIds.emplace_back(it.mMailDBId);
		
		if (++count >= MAX_MAIL_READ_COUNT)
			break;
	}

	GatewaySession* gatewaySession = GetGatewaySession();
	gatewaySession->Send_CG_REQ_MAIL_READ(authSessionContext->mAccountId, MailBoxType::ACCOUNT, mailDBIds);

	for (const auto& it : mailDBIds)
	{
		GConsolePrinter->OutConsoleAsync(Color::MAGENTA, L"account %llu - 메일 받기 요청 : %llu", authSessionContext->mAccountId, it);
	}	

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::TaskHandler_COMPLETE_SCENARIO(
	AuthSessionContext* authSessionContext,
	GatewaySessionContext* gatewaySessionContext
)
{
	TRACE;

	GConsolePrinter->OutConsoleAsync(Color::MAGENTA, L"ScenarioMail::TaskHandler_COMPLETE_SCENARIO()\n");

	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != authSessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != gatewaySessionContext);

	EndScenario();

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::PacketHandler_SC_ACK_MAIL_LIST(
	const uchar* buf, int len,
	AuthSessionContext* authSessionContext,
	GatewaySessionContext* gatewaySessionContext
)
{
	TRACE;

	GConsolePrinter->OutConsoleAsync(Color::BLUE, L"ScenarioMail::PacketHandler_SC_ACK_MAIL_LIST()\n");
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != authSessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != gatewaySessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(gatewaySessionContext->mGatewaySessionState == GatewaySessionContext::GATEWAY_SESSION_STATE::LOGINED);

	BOT_READ_WZ_PACKET(buf, len, PD::SC::PKT_SC_ACK_MAIL_LIST_READ);

	PD::CS::CSResult result = static_cast<PD::CS::CSResult>(pkt->Result());
	if (result != PACKET_RESULT_SUCCESS)
	{
		const wchar_t* resultText = PD::CS::CSResultEnumToString(result);
		GConsolePrinter->OutConsoleAsync(Color::RED, L"-pkt->Result() != PACKET_RESULT_SUCCESS, %s\n", resultText);

		mTaskHelper.CompleteTask();
		EndTask();
		return;
	}

	const uchar* mailDataBuffer = pkt->MailDataList();
	int mailDataSize = pkt->MailDataListSize();

	MailDataList mailDataList;
	MakeBuffer2Vector<MailData>(mailDataBuffer, mailDataSize, mailDataList);
	for (MailData& mailData: mailDataList) {
		gatewaySessionContext->mMailBoxLists[pkt->BoxType()].emplace_back(mailData);
	}
	
	mTaskHelper.CompleteTask();
	EndTask();

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::PacketHandler_SC_ACK_MAIL_SEND(
	const uchar* buf, int len,
	AuthSessionContext* authSessionContext,
	GatewaySessionContext* gatewaySessionContext
)
{
	TRACE;

	GConsolePrinter->OutConsoleAsync(Color::BLUE, L"ScenarioMail::PacketHandler_SC_ACK_MAIL_SEND()\n");

	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != authSessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != gatewaySessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(mTaskHelper.CheckState(TaskIdx::CS_REQ_MAIL_SEND, TaskState::REQUESTED));

	BOT_READ_WZ_PACKET(buf, len, PD::SC::PKT_SC_ACK_MAIL_SEND_READ);

	PD::CS::CSResult result = static_cast<PD::CS::CSResult>(pkt->Result());
	if (result != PACKET_RESULT_SUCCESS)
	{
		const wchar_t* resultText = PD::CS::CSResultEnumToString(result);
		GConsolePrinter->OutConsoleAsync(Color::RED, L"-pkt->Result() != PACKET_RESULT_SUCCESS, %s\n", resultText);

		mTaskHelper.CompleteTask();
		EndTask();
		return;
	}
	
	// 생성된 우편을 추가
	gatewaySessionContext->mMailBoxLists[static_cast<size_t>(pkt->Mail().mMailBoxType)].emplace_back(pkt->Mail());

	mTaskHelper.CompleteTask();
	EndTask();

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ScenarioMail::PacketHandler_SC_ACK_MAIL_READ(
	const uchar* buf, int len,
	AuthSessionContext* authSessionContext,
	GatewaySessionContext* gatewaySessionContext
)
{
	TRACE;

	GConsolePrinter->OutConsoleAsync(Color::BLUE, L"ScenarioMail::PacketHandler_SC_ACK_MAIL_READ()\n");

	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != authSessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(nullptr != gatewaySessionContext);
	_OUT_CONSOLE_FILE_FATAL_ERROR_DEFAULT(mTaskHelper.CheckState(TaskIdx::CS_REQ_MAIL_READ, TaskState::REQUESTED));

	BOT_READ_WZ_PACKET(buf, len, PD::SC::PKT_SC_ACK_MAIL_READ_READ);

	PD::CS::CSResult result = static_cast<PD::CS::CSResult>(pkt->Result());
	if (result != PACKET_RESULT_SUCCESS)
	{
		const wchar_t* resultText = PD::CS::CSResultEnumToString(result);
		GConsolePrinter->OutConsoleAsync(Color::RED, L"-pkt->Result() != PACKET_RESULT_SUCCESS, %s\n", resultText);

		mTaskHelper.CompleteTask();
		EndTask();
		return;
	}

	uint16 boxType = pkt->BoxType();	

	pkt->ForEachReadMailDBIds<MailDBId>([&](const MailDBId* mailDBId)
		{
			GConsolePrinter->OutConsoleAsync(Color::BLUE, L"account %llu - 메일 받기 완료 : %llu", authSessionContext->mAccountId, *mailDBId);
			std::EraseIf(gatewaySessionContext->mMailBoxLists[boxType], [mailDBId](MailData& it) {
				return it.mMailDBId == *mailDBId;
				});
		});

	ItemDataList itemDataCreated;
	pkt->MakeItemDataCreatedVector<ItemDataList, ItemData>(itemDataCreated);

	ItemDBIdCountPairList itemIncreased;
	pkt->MakeItemIncreasedVector<ItemDBIdCountPairList, ItemDBIdCountPair>(itemIncreased);

	gatewaySessionContext->AddItems(itemDataCreated);
	gatewaySessionContext->UpdateItems(itemIncreased);

	mTaskHelper.CompleteTask();
	EndTask();

	TRACE_END;
}