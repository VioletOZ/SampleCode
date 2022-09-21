#include "MailBox/MailBox.h"

#include "User/User.h"
#include "User/UserManager.h"

#include "GatewayServerConfig.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MailBox::MailBox(MailBoxType boxType) : mMailBoxType(boxType)
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MailBox::AddMail(const MailData& mail)
{
	TRACE;

	if (false == mMailDBIdSet.contains(mail.mMailDBId))
	{
		mMailDBIdSet.emplace(mail.mMailDBId);
		mMailDataList.emplace_back(mail);
	}

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MailBox::DeleteMail(MailDBId mailDBId)
{
	TRACE;

	_ASSERT_CRASH(true == mMailDBIdSet.contains(mailDBId))

	mMailDBIdSet.erase(mailDBId);
	std::EraseIf(mMailDataList, [mailDBId](MailData& it) {
		return mailDBId == it.mMailDBId;
		});

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MailBox::CacheMailList(const MailDataList& mailDataList)
{
	TRACE;

	mCached.store(true);

	for (const MailData& mailData : mailDataList)
	{
		AddMail(mailData);
	}

	TRACE_END;
}
