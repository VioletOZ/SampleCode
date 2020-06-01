#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MailBox BaseClass
// Gateway���� �����ϴ� ����������(AccountMailBox)�� ĳ���Ϳ�����(UserMailBox)�� �θ� Ŭ����
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Inventory;

class MailBox
{
public:
	const MailBoxType	mMailBoxType;
	

private:
	MailDBIdSet			mMailDBIdSet;		// ���� �ߺ� ���� üũ�뵵
	MailDataList		mMailDataList;		// �����Կ� ĳ���ص� ���� ����
	std::atomic<bool>	mCached = false;


public:
	explicit MailBox(MailBoxType boxType);


public:
	// ������ ĳ���Ѵ�
	void CacheMailList(const MailDataList& mailDataList);
	bool IsCached() const noexcept { return mCached.load(); }


	// getter
public:
	MailDataList& GetMailDataList() noexcept { return mMailDataList; }
	const MailDataList& GetMailDataList() const noexcept { return mMailDataList; }
	

public:
	void AddMail(const MailData& mail);
	void DeleteMail(MailDBId mailDBId);
};
