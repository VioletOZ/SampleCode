#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MailBox BaseClass
// Gateway에서 관리하는 계정우편함(AccountMailBox)과 캐릭터우편함(UserMailBox)의 부모 클래스
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Inventory;

class MailBox
{
public:
	const MailBoxType	mMailBoxType;
	

private:
	MailDBIdSet			mMailDBIdSet;		// 우편 중복 여부 체크용도
	MailDataList		mMailDataList;		// 우편함에 캐시해둔 메일 정보
	std::atomic<bool>	mCached = false;


public:
	explicit MailBox(MailBoxType boxType);


public:
	// 우편을 캐시한다
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
