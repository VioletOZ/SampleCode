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
	/* �̺κ��� ���� ���� �ޱ� �׽�Ʈ�� �ּ��� Ǯ�� �׽�Ʈ ����
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

	// ������ ���� 0 = ACCOUNT, 1 = USER
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

	// �׽�Ʈ������ �ϴ� �������������� ��������. ������ ������ �׽�Ʈ.
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
		
	// �����δ� Ŭ�� �������� MailDBId�� ��Ƽ� �ޱ� ��û�ϵ��� �� �����Դϴ�
	// ������ ��Ʈ������ �׽�Ʈ�� �׳� �ڽ��� ���� ������ ���� �ޱ� �õ��ϴ� ������ �����մϴ�(�ִ� 20��)
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
		GConsolePrinter->OutConsoleAsync(Color::MAGENTA, L"account %llu - ���� �ޱ� ��û : %llu", authSessionContext->mAccountId, it);
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
	
	// ������ ������ �߰�
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
			GConsolePrinter->OutConsoleAsync(Color::BLUE, L"account %llu - ���� �ޱ� �Ϸ� : %llu", authSessionContext->mAccountId, *mailDBId);
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