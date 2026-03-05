#include "stdafx.h"
#include "AutoPacketMsg.h"
#include "table/TableItemReqList.h"
#include "table/TableBalance.h"
#include "table/TableCharacter.h"
#include "table/TableCharacterSkillPassive.h"
#include "table/TableItem.h"
#include "table/TableCard.h"
#include "table/TableCardDispatchSlot.h"
#include "table/TableCardDispatchMission.h"
#include "table/TableEventExchangeReward.h"
#include "table/TableEventSet.h"
#include "table/TableFacility.h"
#include "table/TableFacilityItemCombine.h"
#include "table/TableStore.h"
#include "table/TableStoreRoom.h"
#include "table/TableRandom.h"
#include "table/TableRoomFigure.h"
#include "table/TableRoomAction.h"
#include "table/TableRoomFunc.h"
#include "table/TableRoomTheme.h"
#include "table/TableAchieve.h"
#include "table/TableCostume.h"
#include "table/TableLevelUp.h"
#include "table/TableBadgeOpt.h"
#include "table/TableArenaGrade.h"
#include "table/TableGem.h"
#include "table/TablePassSet.h"
#include "table/TableStage.h"

#include "other/ProjectMgr.h"

auto trueReturnWork = [](const DataItem&) { return true; };

bool AutoPacketMsg::ChkCanUseItemByTIDAndGetUID(c_uint32 _itemTID, c_uint32 _itemCnt, uint64_t* _outUID) const
{
    const ITEM_VECMAP& itemList{ m_DummyUserInfo->GetItemList() };
    c_size_t itemSize{ itemList.size() };
    if (itemSize <= 0)  return false;

	for (size_t idx{}; idx < itemSize; ++idx)
	{
        const ItemData& itemData{ itemList.GetDataWithIdx(idx) };
        if (itemData.TID == _itemTID)
        {
            if (itemData.count >= _itemCnt)
            {
                if (_outUID != nullptr)
					*_outUID = itemData.UID;
                return true;
            }

            return false;
        }
    }

    return false;
}

const DataItemReqList & AutoPacketMsg::GetItemReqData(c_uint32 _group, c_uint32 _lv) const
{
    if (_group == 0)
		return DataItemReqList::ms_Null;
    const DataItemReqList& data{ TableItemReqList::Inst->GetData(_group,_lv) };
    return data;
}

bool AutoPacketMsg::CheckItemGroup(const DataItemReqList & _itemReqData, int32_t _chkLevel, uint32_t _goods) const
{
    const GOODS_ARR& goodsData{ m_DummyUserInfo->GetGoodsData() };

    if (_itemReqData.Gold > goodsData[eGOODSTYPE::GOLD])
        return false;

    if (_itemReqData.LimitLevel > _chkLevel)
        return false;

    if (_itemReqData.GoodsValue > _goods)
        return false;

    uint8_t itemExistCount = 0;
    c_size_t size{ _itemReqData.reqVec_.size() };
    if (size == 0)
		return true;

    for (const auto& info : _itemReqData.reqVec_)
    {
        if (true == ChkCanUseItemByTIDAndGetUID(info.ID, info.Cnt))   
            ++itemExistCount;
    }

    if (itemExistCount >= size)
		return true;

    return false;
}

bool AutoPacketMsg::CheckAlreadyFacilityOper(c_uint64 _operValue) const 
{
    const FACILITY_VECMAP& facilityList{ m_DummyUserInfo->GetFacilityList() };
    c_size_t facilitySize{ facilityList.size() };
    for (uint8_t facilityIdx = 0; facilityIdx < facilitySize; ++facilityIdx)
    {
        const FacilityData& data = facilityList.GetDataWithIdx(facilityIdx);
        if (data.operValue == _operValue)
            return false;
    }
    return true;
}

bool AutoPacketMsg::GetMaterItems(PktInfoItemCntVec * _outData, eITEMSUBTYPE _itemType, LAMBDA(bool(const DataItem &_data), _work)) const
{
    const ITEM_VECMAP& itemList{ m_DummyUserInfo->GetItemList() };
    c_size_t itemSize{ itemList.size() };
    auto itemWork       = [&_outData, &_itemType, &_work, &itemSize, &itemList](c_uint64 /*idx*/, const DataDefault& _data) {

                        const DataItem& item = static_cast<const DataItem&>(_data);
                        if (false == item.IsCanUseMaterial(_itemType) || false == _work(item))
							return false;

                        for (int itemIdx{ 0 }; itemIdx < itemSize; ++itemIdx)
                        {
                            const ItemData& data{ itemList.GetDataWithIdx(itemIdx) };
                            if (data.TID != item.ID)
								continue;
                            if (0 >= data.count)
								return false;

                            _outData->DoAdd(data.UID, 1);
                            return true;
                        }

                        return false;
                    };
    return TableItem::Inst->DoWorkToDatas(itemWork);
}

bool AutoPacketMsg::CheckCanAddUserItem() const
{
    const FACILITY_VECMAP& facilityList{ m_DummyUserInfo->GetFacilityList() };
    const WEAPON_VECMAP& weaponList{ m_DummyUserInfo->GetWeaponList() };
    const GEM_VECMAP& gemList{ m_DummyUserInfo->GetGemList() };
    const ITEM_VECMAP& itemList{ m_DummyUserInfo->GetItemList() };
    uint16_t itemSlotCnt{ m_DummyUserInfo->GetUserData().itemSlotCnt };

    uint16_t cntItem{};
    c_size_t itemSize{ itemList.size() };
    for (uint16_t itemIdx{ 0 }; itemIdx < itemSize; ++itemIdx)
    {
        if (true == itemList.GetDataWithIdx(itemIdx).GetIsAddSlotCount())
            ++cntItem;
    }

    uint16_t userItemCnt{ static_cast<uint16_t>(facilityList.size() + weaponList.size() + gemList.size() + cntItem) };
    return itemSlotCnt > userItemCnt;
}

bool AutoPacketMsg::GetCanReflashLoginBonus() const
{
    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    return userData.CheckCanGetLoginBonus(ProjectMgr::ms_Inst->GetNowDateTimeNum());
}

bool AutoPacketMsg::ChkAndGetAchieveRewardTakeGroupID(PktInfoTIDList* _idList) const
{
    const ACHIEVE_VECMAP& achiveList{ m_DummyUserInfo->GetAcheveList() };
    c_size_t achieveSize{ achiveList.size() };

    uint32_t addGroupID{};
    for (uint8_t achieveIdx{ 0 }; achieveIdx < achieveSize; ++achieveIdx)
    {
        const AchieveData& achieveInfo{ achiveList.GetDataWithIdx(achieveIdx) };
        const DataAchieve& achieveData{ *achieveInfo.GetTableData() };
        if (achieveData.maxInfo->maxOrder_ < achieveInfo.nowStep)
            continue;

        addGroupID      = 0;
        switch (achieveData.AchieveType)
        {
            case eAchieveType::AM_Adv_StoryClear: {
                const CLEAR_STAGE_VECMAP& stageClearList{ m_DummyUserInfo->GetClearStageList() };
                size_t stageClearSize{ stageClearList.size() };
                for (uint8_t stageIdx{ 0 }; stageIdx < stageClearSize; ++stageIdx)
                {
                    const ClearStageData& clearData{ stageClearList.GetDataWithIdx(stageIdx) };
                    if (clearData.TID == achieveData.AchieveIndex) 
                    {
                        addGroupID = achieveData.GroupID;
                        break;
                    }
                }
            } break;
            case eAchieveType::AM_Gro_UserRank: {
                const UserData& userData{ m_DummyUserInfo->GetUserData() };
                if (userData.expLv.lv_ < achieveData.AchieveValue)
                    continue;
                addGroupID  = achieveData.GroupID;
            } break;
            case eAchieveType::AM_Gro_CharLv: {
				const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
				if (false == charList.CanGetDataWithKey(achieveData.AchieveIndex))
					break;

				const CharData& charData{ charList.GetDataWithKey(achieveData.AchieveIndex) };
				if (charData.lv >= achieveData.AchieveValue)
				{
					addGroupID = achieveData.GroupID;
					break;
				}
            } break;
            case eAchieveType::AM_Gro_CharGrade: {
				const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
				if (false == charList.CanGetDataWithKey(achieveData.AchieveIndex))
					break;

				const CharData& charData{ charList.GetDataWithKey(achieveData.AchieveIndex) };
				if (charData.grade >= achieveData.AchieveValue)
				{
					addGroupID = achieveData.GroupID;
					break;
				}
            } break;
            default: {
                if (achieveInfo.condiVal < achieveData.AchieveValue)
                    continue;
                addGroupID  = achieveData.GroupID;
            } break;
        }

        if (0 == addGroupID)
            continue;
		else
		{
			_idList->DoAdd(addGroupID);
			break;
		}
    }
    if (true == _idList->IsEmpty())
        return false;
    return true;
}

bool AutoPacketMsg::ChkAndGetWeekMissionRewardIdx(uint8_t & _outIdx) const
{
    const WeekMissionData& weekMissionData{ m_DummyUserInfo->GetWeekMissionData() };
    _outIdx             = weekMissionData.GetOnRewardIdx();
    if (PktInfoMission::Weekly::ENUM::_MAX_ > _outIdx)
		return true;

    return false;
}
bool AutoPacketMsg::ChkAndGetGllaMissionUpdate(PktInfoUpdateGllaMission& _outPkt) const
{
    PktInfoUpdateGllaMission::Piece addInfo{ 1, 2 };
    if (true == m_DummyUserInfo->GetGllaMissionData().CanGetDataWithKey(addInfo.type_))
        return false;

    _outPkt.infos_.push_back(addInfo);
    return true;
}
bool AutoPacketMsg::ChkAndGetGllaMissionRewardIdx(uint32_t& _outIdx) const
{
    const PktInfoTblSvrGuerrillaMission& gllaMissonList{ m_DummyUserInfo->GetGllaMissionList() };
    const GLLA_MISSION_VECMAP& gllaMissionData{ m_DummyUserInfo->GetGllaMissionData() };

    c_size_t gllaSize{ gllaMissionData.size() };
    for (const auto& mission : gllaMissonList.infos_)
    {
        const GllaMissionData& data{ gllaMissionData.GetDataWithKey(mission.GroupID) };

        if (0 == data.groupID || 0 == STRCMP(mission.Type.c_str(), _TX("GM_LoginBonus")))
            continue;

        if (mission.GroupOrder == data.step && mission.Count <= data.count)
        {
            _outIdx = mission.GroupID;
            return true;
        }
    }

    return false;
}

bool AutoPacketMsg::ChkAndGetRewardPassMissionPkt(PktInfoRewardPassMission& _outPkt) const
{
	const PASS_MISSION_VECMAP& passMissionData{ m_DummyUserInfo->GetPassMissionData() };

	c_size_t passSize{ passMissionData.size() };
	for (size_t idx{}; idx < passSize; ++idx)
	{
		const PassMissionData& data{ passMissionData.GetDataWithIdx(idx) };

		if (data.state == 0 && data.value == 0)
		{
			_outPkt.ids_.DoAdd(data.ID);
			return true;
		}
	}
	return false;
}

bool AutoPacketMsg::ChkAndGetRewardPassPkt(PktInfoRewardPassReq& _outPkt) const
{
	const PktInfoUserPass& userPass{ m_DummyUserInfo->GetUserData().pass };
	c_uint32 userPassPt{ m_DummyUserInfo->GetUserData().nowPassPt };
	auto work			= [&_outPkt, &userPass, &userPassPt](c_uint64 /*_idx*/, const DataDefault& _data)
	{
						const DataPassSet& passData{ static_cast<const DataPassSet&>(_data) };
						if (false == passData.IsActive(ProjectMgr::ms_Inst->GetNowDateTimeNum()))
							return false;

						auto randWork   = [&_outPkt, &userPass, &passData, &userPassPt](const DataRandom& _data, c_uint64 /*_idx*/) {
									
											if (_data.Value <= userPassPt && userPass.step_N_ < _data.Value)
											{
												_outPkt.passID_ = passData.ID;
												_outPkt.rwdEndPT_N_ = static_cast<uint8_t>(_data.Value);
												return true;
											}
											return false;
										};
						if (TableRandom::Inst->DoWorkAllToGroupDatas(passData.N_RewardID, randWork))
							return true;

						return false;
	};
	if (true == TablePassSet::Inst->DoWorkToDatas(work))
		return true;

	return false;
}

bool AutoPacketMsg::ChkAndGetRewardEventResetID(uint32_t & _outID) const
{
    const EVENT_VEC& eventList{ m_DummyUserInfo->GetEventList() };
    for (const auto& eventData : eventList)
    {
        if (true == eventData.CanReset(eEventRewardKind::RESET_LOTTERY))
        {
            _outID  = eventData.TID;
            return true;
        }
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetEventRewardReqPkt(PktInfoEventRewardReq & _outPkt) const
{
    const EVENT_VEC& eventList{ m_DummyUserInfo->GetEventList() };
    for (const auto& eventData : eventList)
    {
        switch (eventData.type)
        {
        case eEventRewardKind::RESET_LOTTERY :
        {
            if (true == this->ChkCanUseItemByTIDAndGetUID(eventData.GetEvtItemID1(), TableBalance::Inst->Get().EvtResetGachaReqCnt))
            {
                _outPkt.eventID_    = eventData.TID;
                _outPkt.step_       = eventData.step;
                _outPkt.cnt_        = eventData.ChkAllZero();
                return true;
            }
        }   break;
        case eEventRewardKind::EXCHANGE:
        {
            for (size_t loop{}; loop < PktInfoEventReward::ENUM::_MAX_; ++loop)
            {
                c_uint8 idx{ static_cast<c_uint8>(loop) };
                const DataEventExchangeReward& dataRwd{ TableEventExchangeReward::Inst->GetData(eventData.TID, eventData.step, idx) };
                if (true == dataRwd.IsNull())
                    break;

                if (false == dataRwd.IsInfiniteRwd() && 0 == eventData.counts[loop])
                    continue;

                bool reqItemOK{true};
                for (const auto& info : dataRwd.reqVec_)
                {
                    if (false == this->ChkCanUseItemByTIDAndGetUID(info.ID, info.Cnt))
                    {
                        reqItemOK   = false;
                        break;
                    }
                }
                if (true == reqItemOK)
                {
                    _outPkt.eventID_    = eventData.TID;
                    _outPkt.step_       = eventData.step;
                    _outPkt.idx_        = idx;
                    _outPkt.cnt_        = eventData.ChkAllZero();
                    return true;
                }
            }
        }   break;
        }
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetAddCharTID(uint32_t & _outTID) const
{
    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
    c_size_t charSize{ charList.size() };
    if (0 < charSize)
		return false;

	// ReqAddCharacter 패킷을 통해서는 아사기, 사쿠라, 유키카제 중 하나의 캐릭터만 얻을 수 있습니다.
    _outTID             = static_cast<uint32_t>(ProjectMgr::ms_Inst->GetRand(1, 3));
    
    return true;
}

bool AutoPacketMsg::ChkAndGetMainCharIdx(Cuid_t& _outCuid) const
{
    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
    c_size_t charSize{ charList.size() };
    if (1 >= charSize)
		return false;

    const UserData& userData{ m_DummyUserInfo->GetUserData() };
 //   c_size_t selIdx{ ProjectMgr::ms_Inst->GetRand(0, charSize - 1) };
	//uint64_t cuid{ charList.GetDataWithIdx(selIdx).CUID };
 //   if (userData.mainCharUID == cuid)
 //       return false;

	m_Idx.DoStart(charSize - 1, _MAC_FUNC_);
	do {
		uint64_t cuid{ charList.GetDataWithIdx(m_Idx.GetCurIdx()).CUID };
		if (userData.charUID != cuid)
		{
			_outCuid = cuid;
			return true;
		}
	} while (false == m_Idx.ChangeAndCheckEndIdx());

	return false;
}

bool AutoPacketMsg::ChkAndGetGradeUpCharIdx(Cuid_t& _outCuid) const
{
    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
    c_size_t charSize{ charList.size() };
    if (0 >= charSize)
		return false;

    for (uint8_t idx{ 0 }; idx < charSize; ++idx)
    {
        const CharData& charData{ charList.GetDataWithIdx(idx) };
        uint8_t charGrade   = charData.grade;
        if (TableBalance::Inst->Get().CharMaxGrade > charGrade && TableCharacter::Inst->GetCharMaxLevel(charGrade) <= charData.lv)
        {
            const DataItemReqList& itemReqGroup{ GetItemReqData(charData.GetItemReqGroup(), charGrade) };
            if (CheckItemGroup(itemReqGroup, charGrade))
            {
                _outCuid    = charData.CUID;
                return true;
            }
        }
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetSetMainCostumePkt(PktInfoCharSetMainCostume & _outPkt) const
{
    const COSTUME_VEC& costumeList{ m_DummyUserInfo->GetCostumeList() };
    c_size_t cosSize{ costumeList.size() };
    if (0 >= cosSize)
		return false;

    m_Idx.DoStart(cosSize - 1, _MAC_FUNC_);
    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
    do {
        c_uint32 nowCostimeTID{ costumeList[m_Idx.GetCurIdx()] };
        const DataCostume& costumeData{ TableCostume::Inst->GetData(nowCostimeTID) };

		if (false == charList.CanGetDataWithKey(costumeData.CharacterID))
			continue;

		const CharData& charData{ charList.GetDataWithKey(costumeData.CharacterID) };
		if (nowCostimeTID != charData.mainCostumeID)
		{
			_outPkt.skinColor_.cuid_ = charData.CUID;
			_outPkt.costumeTID_ = nowCostimeTID;

			return true;
		}
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    return false;
}

bool AutoPacketMsg::ChkAndGetEquipWeaponPkt(PktInfoCharEquipWeapon & _outPkt) const
{
    const WEAPON_VECMAP& weaponList{ m_DummyUserInfo->GetWeaponList() };

    c_size_t weaponSize{ weaponList.size() };
    if (0 >= weaponSize)
		return false;

    m_Idx.DoStart(weaponSize - 1, _MAC_FUNC_);

    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
    c_size_t maxChar{ charList.size() };
    do {
        const WeaponData& weaponData{ weaponList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        if (INVALID_UID == weaponData.equipCharUID && true == CheckAlreadyFacilityOper(weaponData.UID))
        {
			if (false == charList.CanGetDataWithKey(weaponData.GatWeaponCharID()))
				continue;

			const CharData& charData{ charList.GetDataWithKey(weaponData.GatWeaponCharID()) };

			uint8_t idx{ static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, 1)) };
			_outPkt.skinColor_.cuid_ = charData.CUID;
			_outPkt.wpns_[idx].wpnUID_ = weaponData.UID;
			if (1 == idx)
				_outPkt.wpns_[0].wpnUID_ = charData.wpnUIDs[0];
			else
				_outPkt.wpns_[1].wpnUID_ = charData.wpnUIDs[1];

			return true;
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    return false;
}

bool AutoPacketMsg::ChkAndGetSkillSlotPkt(PktInfoCharSlotSkill & _outPkt) const
{
    const SKILL_VECMAP& skillList{ m_DummyUserInfo->GetSkillList() };
    c_size_t skillSize{ skillList.size() };
    if (0 >= skillSize)
		return false;

    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
    const CharData* selChar{};
	uint32_t charExSkill{};

	uint8_t slotIdx{};
	m_Idx.DoStart(skillSize - 1, _MAC_FUNC_);
	do {
		const SkillData& skillData = skillList.GetDataWithIdx(m_Idx.GetCurIdx());
		if (INVALID_UID != skillData.equipCharUID)
			continue;


		const CharData* findCharData{ &charList.GetDataWithKey(skillData.GatSkillCharID()) };
		if (true == skillData.CanEquipSkill(static_cast<uint8_t>(eCHARSKILLPASSIVETYPE::SELECT_NORMAL), findCharData->TID))
		{
			if (nullptr != selChar)
			{
				if (true == skillData.CanEquipSkill(static_cast<uint8_t>(eCHARSKILLPASSIVETYPE::SELECT_NORMAL), selChar->TID))
					_outPkt.skilTIDs_[slotIdx] = static_cast<uint32_t>(skillData.skillTID);
			}
			else
			{
				selChar = findCharData;
                if (true == m_DummyUserInfo->GetEffectData().IsActiveBuffTP(eBuffType::Buff_SkillSlot))
				    charExSkill = findCharData->skillIDs[PktInfoChar::SkillSlot::_BUFF_SKILL];
				_outPkt.cuid_ = findCharData->CUID;
				_outPkt.skilTIDs_[slotIdx] = static_cast<uint32_t>(skillData.skillTID);
			}
			++slotIdx;
			if (PktInfoChar::SkillSlot::_BUFF_SKILL < slotIdx)
				break;
		}

	} while (false == m_Idx.ChangeAndCheckEndIdx());

	if (nullptr != selChar)
	{
		_outPkt.skilTIDs_[PktInfoChar::SkillSlot::_BUFF_SKILL] = charExSkill;
		return true;
	}

    return false;
}

bool AutoPacketMsg::ChkAndGetLvUpSkillID(PktInfoSkillLvUpReq& _outPkt) const
{
    const SKILL_VECMAP& skillList{ m_DummyUserInfo->GetSkillList() };
    if (0 >= skillList.size())
		return false;

    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
    c_size_t charSize{ charList.size() };
	if (charSize <= 0)
		return false;

	auto _work = [this, &charList, &_outPkt, &skillList](c_uint64, const DataDefault& _data)
	{
		const DataCharacterSkillPassive& skillData = static_cast<const DataCharacterSkillPassive&>(_data);
		
		uint8_t skillLv{ skillList.GetDataWithKey(skillData.ID).lv };
		if (skillLv == 0 && (false == skillList.CanGetDataWithKey(skillData.ParentID) && -1 != skillData.ParentID))
			return false;

		if (false == charList.CanGetDataWithKey(skillData.CharacterID))
			return false;

		const CharData& charData{ charList.GetDataWithKey(skillData.CharacterID) };

		if (skillLv < skillData.MaxLevel)
		{
			const DataItemReqList& itemReqData = GetItemReqData(skillData.ItemReqListID, skillLv);

			if (false == itemReqData.IsNull() && CheckItemGroup(itemReqData, charData.lv, charData.skillPT))
			{
				_outPkt.skillTID_	= skillData.ID;
				_outPkt.cuid_		= charData.CUID;
				return true;
			}
		}

		return false;
	};
	if (true == TableCharacterSkillPassive::Inst->DoWorkToDatas(_work))
		return true;
    return false;
}

bool AutoPacketMsg::ChkAndGetStartStagePkt(PktInfoStageGameStartReq& _outPkt) const
{
	if (false == CheckCanAddUserItem())
		return false;

	const GOODS_ARR& goodsData{ m_DummyUserInfo->GetGoodsData() };
	if (3 > goodsData[eGOODSTYPE::TICKET] && 1 > goodsData[eGOODSTYPE::BATTLETICKET]) return false;

	const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
	if (charList.size() <= 0)
		return false;

	c_uint64 charIdx{ ProjectMgr::ms_Inst->GetRand(0, charList.size() - 1) };
	_outPkt.cuid_ = charList.GetDataWithIdx(charIdx).CUID;

	for (size_t randIdx{}; randIdx < 5; ++randIdx)
	{
		auto _work = [this, &_outPkt, &goodsData](const DataDefault& _data)
		{
			const DataStage& stageData = static_cast<const DataStage&>(_data);

			eGOODSTYPE ticketType{ eGOODSTYPE::TICKET };

			if (eSTAGETYPE::STAGE_SPECIAL == stageData.StageType)
			{
				const StageData::Special& special{ m_DummyUserInfo->GetStageData().special };
				if (special.stageTID != stageData.ID || false == special.nextTime.IsAbove(ProjectMgr::ms_Inst->GetNowDateTimeNum()))
					return;
			}
			else if (eSTAGETYPE::STAGE_TIMEATTACK == stageData.StageType)
				ticketType = eGOODSTYPE::BATTLETICKET;
			else if (eSTAGETYPE::STAGE_EVENT == stageData.StageType)
			{
				const DataEventSet& eventdata{ TableEventSet::Inst->GetData(stageData.TypeValue) };
				if (false == eventdata.IsCanPlay(ProjectMgr::ms_Inst->GetNowDateTimeNum()))
					return;
			}

			if (stageData.Ticket <= goodsData[ticketType])
			{
				_outPkt.stageTID_ = stageData.ID;
				return;
			}
			return;
		};
		TableStage::Inst->DoWorkToRandData(_work);
	}
	if (_outPkt.stageTID_ == 0)
		return false;
	return true;
}

bool AutoPacketMsg::ChkAndGetStageEndPkt(PktInfoStageGameResultReq & _outPkt) const
{
    const StageInfo& stageInfo{ m_DummyUserInfo->m_nowStage };
    if (stageInfo.stageTID == 0)
		return false;

    const CLEAR_STAGE_VECMAP& clearStageList{ m_DummyUserInfo->GetClearStageList() };
    _outPkt.certifyKey_         = stageInfo.certifyKey;

    _outPkt.nTakeBoxCnt_        = static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, stageInfo.boxMaxCnt));
    _outPkt.clearTime_Ms        = static_cast<uint32_t>(ProjectMgr::ms_Inst->GetRand(100000, 200000));
    _outPkt.dropGoldItemCnt_    = static_cast<uint16_t>(ProjectMgr::ms_Inst->GetRand(1, 100));

    size_t maxCount{ clearStageList.size() };
    for (int stageIdx = 0; stageIdx < maxCount; ++stageIdx)
    {
        const ClearStageData& stageData{ clearStageList.GetDataWithIdx(stageIdx) };
        if (stageInfo.stageTID == stageData.TID)
        {
            _outPkt.mission0_ = stageData.IsOnReward(0) ? false : true;
            if (true == _outPkt.mission0_)
				return true;
            _outPkt.mission1_ = stageData.IsOnReward(1) ? false : true;
            if (true == _outPkt.mission1_)
				return true;
            _outPkt.mission2_ = stageData.IsOnReward(2) ? false : true;
            return true;
        }
    }

    return true;
}

bool AutoPacketMsg::ChkAndGetStageFailPkt(PktInfoStageGameEndFail & _outPkt) const
{
    const StageInfo& stageInfo{ m_DummyUserInfo->m_nowStage };
    if (stageInfo.stageTID == 0)
		return false;

    _outPkt.certifyKey_     = stageInfo.certifyKey;
    _outPkt.playTime_Ms_    = (uint32_t)ProjectMgr::ms_Inst->GetRand(5, 10) * 10000;

    return true;
}

bool AutoPacketMsg::ChkAndGetTimeAtkRankingHeaderPkt(PktInfoTimeAtkRankingHeader & _outPkt) const
{
    auto _work          = [&_outPkt](const DataDefault &_data)
                    {
                        const DataStage& stageData = static_cast<const DataStage&>(_data);
                        if (true == stageData.IsCanTimeAtkRecode())
                        {
                            PktInfoTimeAtkRankingHeader::Piece header{};
                            header.stageID_ = stageData.ID;
                            _outPkt.infos_.push_back(MOVE_CAST(header));
                            return true;
                        }

                        return false;
                    };
    TableStage::Inst->DoWorkAllToDatas(_work);
    return true;
}

bool AutoPacketMsg::ChkAndGetTimeAtkRankerDetailPkt(PktInfoTimeAtkRankerDetailReq & _outPkt) const
{
    PktInfoTimeAtkRankStageList rankList{ m_DummyUserInfo->GetTimeAtkRankingList() };
    c_size_t rankSize{ rankList.infos_.size() };
    if (rankSize <= 0)
		return false;

    c_uint64 stageIdx{ ProjectMgr::ms_Inst->GetRand(0, rankSize - 1) };
    PktInfoTimeAtkRankStage stageRankList{ rankList.infos_[stageIdx] };
    c_uint64 userIdx{ ProjectMgr::ms_Inst->GetRand(0, 9) };

    _outPkt.stageID_    = stageRankList.header_.stageID_;
    _outPkt.uuid_       = stageRankList.infos_[userIdx].uuid_;

    if (0 == _outPkt.uuid_)
		return false;

    return true;
}

bool AutoPacketMsg::ChkCanArenaSeasonPlay() const
{
    const PktInfoUserArena& arenaInfo{ m_DummyUserInfo->GetArenaInfo() };
    const PktInfoArenaSeasonTime& arenaTime{ m_DummyUserInfo->GetArenaTime() };
    const PktInfoTime nowDateTime{ ProjectMgr::ms_Inst->GetNowDateTimeNum() };

    if (true == nowDateTime.IsAbove(arenaInfo.record_.lastRewardTime_.time_))
        return false;

    if (true == arenaTime.IsReadyTime())
        return false;

    return true;
}

bool AutoPacketMsg::ChkAndGetArenaTeamPkt(PktInfoUserArenaTeam& _outPkt) const
{
	if (false == ChkCanPlayArena())
		return false;

    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
    const PktInfoUserArenaTeam& arenaTeam{ m_DummyUserInfo->GetArenaInfo().team_ };

    size_t charSize{ charList.size() };

    if (charSize <= 0)
        return false;

    int64_t maxIdx{ PktInfoUserArenaTeam::eCHAR::_MAX_ - 1 };
    int64_t minIdx{ static_cast<int64_t>(maxIdx - charSize) + 1 };
    if (minIdx < 0)
        minIdx = 0;
    
    m_Idx.DoStart(charSize - 1, _MAC_FUNC_);

    for (int64_t idx{ maxIdx }; idx >= minIdx; --idx)
    {
        do {
            const CharData& charData{ charList.GetDataWithIdx(m_Idx.GetCurIdx()) };
            if (charData.CUID != arenaTeam.CUIDs_[idx])
            {
				bool sameID{ false };
				for (int64_t loop{ idx }; loop < PktInfoUserArenaTeam::eCHAR::_MAX_; ++loop)
				{
					if (charData.CUID == _outPkt.CUIDs_[loop])
					{
						sameID	= true;
						break;
					}
				}
				if (false == sameID)
				{
					_outPkt.CUIDs_[idx] = charData.CUID;
					break;
				}
            }
        } while (false == m_Idx.ChangeAndCheckEndIdx());
    }
   
    return 0 != _outPkt.CUIDs_[PktInfoUserArenaTeam::eCHAR::THIRD];
}

bool AutoPacketMsg::ChkCanArenaSearchEnemy() const
{
	if (false == ChkCanPlayArena())
		return false;

    const GOODS_ARR goodsList{ m_DummyUserInfo->GetGoodsData() };
	const PktInfoUserArenaRec& arenaRec{ m_DummyUserInfo->GetArenaInfo().record_ };

    uint64_t goldCost{ TableArenaGrade::Inst->GetData(arenaRec.nowGradeID_).MatchPrice };
    if (goldCost < goodsList[eGOODSTYPE::GOLD])
        return true;

    return false;
}

bool AutoPacketMsg::ChkAndGetArenaGameStartPkt(PktInfoArenaGameStartReq& _outPkt) const
{
	if (false == ChkCanPlayArena())
		return false;

    if (false == m_DummyUserInfo->m_ArenaChkFlag)
        return false;

	const PktInfoUserArenaTeam& arenaTeam{ m_DummyUserInfo->GetArenaInfo().team_ };
	if (0 == arenaTeam.CUIDs_[PktInfoUserArenaTeam::eCHAR::THIRD])
		return false;

    const GOODS_ARR goodsList{ m_DummyUserInfo->GetGoodsData() };
    PktInfoItemCntVec itemData{};

    if (1 <= goodsList[eGOODSTYPE::BATTLETICKET])
    {
        if (TableBalance::Inst->Get().ArenaCoinBuffPrice < goodsList[eGOODSTYPE::GOLD])
            _outPkt.useBattleCoinBuff_ = ProjectMgr::ms_Inst->GetRand(0, 1);

        if (true == GetMaterItems(&itemData, eITEMSUBTYPE::MATERIAL_ARENA_ATKBUFF, trueReturnWork))
            _outPkt.useItemUIDs_.push_back(itemData.infos_[0].uid_);
        else if (true == GetMaterItems(&itemData, eITEMSUBTYPE::MATERIAL_ARENA_DEFBUFF, trueReturnWork))
            _outPkt.useItemUIDs_.push_back(itemData.infos_[0].uid_);

        return true;
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetArenaGameEndPkt(PktInfoArenaGameEndReq& _outPkt) const
{
	if (false == ChkCanPlayArena())
		return false;

    if (true == m_DummyUserInfo->m_ArenaChkFlag)
        return false;

    const Proud::Guid& certKey{ m_DummyUserInfo->m_ArenaCertKey };
    Proud::Guid tmp{};

    if(certKey == tmp)
		return false;

	const PktInfoUserArenaRec& arenaRec{ m_DummyUserInfo->GetArenaInfo().record_ };
	uint32_t reqScore{ TableArenaGrade::Inst->GetData(arenaRec.nowGradeID_).ReqScore };
    _outPkt.certifyKey_ = certKey;
    _outPkt.result_		= static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, PktInfoArenaGameEndReq::eRESULT::THIRD_WIN));
	_outPkt.teamPower_	= reqScore + static_cast<uint32_t>(ProjectMgr::ms_Inst->GetRand(1000, 6000));
	_outPkt.enemyScore_	= reqScore + static_cast<uint32_t>(ProjectMgr::ms_Inst->GetRand(0, 1000));
	_outPkt.enemyTeamPower_	= _outPkt.teamPower_ + static_cast<uint32_t>(ProjectMgr::ms_Inst->GetRand(0, 1000));

    return true;
}

bool AutoPacketMsg::ChkCanPlayArena() const
{
	const PktInfoArenaSeasonTime& arenaTime{ m_DummyUserInfo->GetArenaTime() };
	if (PktInfoArenaSeasonTime::STATE::END == arenaTime.seasonState_)
		return false;

	const PktInfoUserArena& arenaInfo{ m_DummyUserInfo->GetArenaInfo() };
	const PktInfoTime nowDateTime{ ProjectMgr::ms_Inst->GetNowDateTimeNum() };
	if (false == nowDateTime.IsAbove(arenaInfo.record_.lastRewardTime_.time_))
		return false;

	return true;
}

bool AutoPacketMsg::GetArenaRankingUpdateTime(datetime_num& _outTime) const
{
	const PktInfoArenaRankList& rankList{ m_DummyUserInfo->GetArenaRankingList() };
	_outTime			= rankList.updateTM_;
    return true;
}

bool AutoPacketMsg::ChkAndGetArenaRankerUUID(Uuid_t& _outUuid) const
{
	const PktInfoArenaRankList& rankList{ m_DummyUserInfo->GetArenaRankingList() };

	c_size_t rankSize{ rankList.infos_.size() };
	if (rankSize <= 0)
		return false;
	c_uint64 idx{ ProjectMgr::ms_Inst->GetRand(0, rankSize - 1) };

	_outUuid			= rankList.infos_[idx].userInfo_.uuid_;
	if (0 == _outUuid)
		return false;

	return true;
}

bool AutoPacketMsg::ChkAndGetArenaTowerTeamPkt(PktInfoUserArenaTeam& _outPkt) const
{
    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
    const PktInfoUserArenaTeam& arenaTeam{ m_DummyUserInfo->GetArenaTowerInfo().team_ };

    size_t charSize{ charList.size() };
    if (charSize <= 0)
        return false;

    int64_t maxIdx{ PktInfoUserArenaTeam::eCHAR::_MAX_ - 1 };
    int64_t minIdx{ static_cast<int64_t>(maxIdx - charSize) + 1 };

    if (minIdx < 0)
        minIdx = 0;

    m_Idx.DoStart(charSize - 1, _MAC_FUNC_);

    for (int64_t idx{ maxIdx }; idx >= minIdx; --idx)
    {
        do {
            const CharData& charData{ charList.GetDataWithIdx(m_Idx.GetCurIdx()) };
            if (charData.CUID != arenaTeam.CUIDs_[idx])
            {
                bool sameID{ false };
                for (int64_t loop{ idx }; loop < PktInfoUserArenaTeam::eCHAR::_MAX_; ++loop)
                {
                    if (charData.CUID == _outPkt.CUIDs_[loop])
                    {
                        sameID = true;
                        break;
                    }
                }
                if (false == sameID)
                {
                    _outPkt.CUIDs_[idx] = charData.CUID;
                    break;
                }
            }
        } while (false == m_Idx.ChangeAndCheckEndIdx());
    }

    if (0 == _outPkt.CUIDs_[PktInfoUserArenaTeam::eCHAR::THIRD])
        return false;

    const UserData& userData{ m_DummyUserInfo->GetUserData() };

    // TODO : Verification CardBook
    _outPkt.cardFrmtID_ = userData.cardFrmtID;

    return true;
}
bool AutoPacketMsg::ChkAndGetArenaTowerGameStartPkt(PktInfoArenaTowerGameStartReq& _outPkt) const
{
    if (false == CanAddItemSlot())
        return false;

    const PktInfoUserArenaTower& arenaTower{ m_DummyUserInfo->GetArenaTowerInfo() };
    if (0 == arenaTower.team_.CUIDs_[PktInfoUserArenaTeam::eCHAR::THIRD])
        return false;

    const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };
    if (0 == friendList.size())
        return false;

    if (PktInfoUserArenaTower::FLOOR::_MAX_ <= arenaTower.info_.claerID_)
        return false;

    _outPkt.towerID_ = arenaTower.info_.claerID_ + 1;

    _outPkt.useCommuUuid_ = friendList.GetDataWithIdx(0).info.uuid;
    
    // 우선 버프는 없이진행 1번 경우 4번째 스킬사용인데 이거는 패키지구매이용자만이라..
    _outPkt.upCharSKBuffFlag_ = 0;

    return true;
}
bool AutoPacketMsg::ChkAndGetArenaTowerGameEndPkt(PktInfoArenaTowerGameEndReq& _outPkt) const
{
    _outPkt.playTime_Ms_ = (uint32_t)ProjectMgr::ms_Inst->GetRand(5, 10) * 10000;
    _outPkt.successFlag_ = ProjectMgr::ms_Inst->GetRand(0, 1);;

    return true;
}

bool AutoPacketMsg::ChkAndGetNewBookConfirmPkt(PktInfoBookNewConfirm& _outPkt) const
{
    const BOOK_VECMAP& weaponBookList{ m_DummyUserInfo->GetWeaponBookList() };
    if (true == ChkBook(_outPkt, weaponBookList, PktInfoWeaponBook::Piece::STATE::NEW_CHK, eBookGroup::Weapon))
		return true;

    const BOOK_VECMAP& cardBookList{ m_DummyUserInfo->GetCardBookList() };
    if (true == ChkBook(_outPkt, cardBookList, PktInfoCardBook::Piece::STATE::NEW_CHK, eBookGroup::Supporter))
		return true;

    const BOOK_VECMAP& monsterBookList{ m_DummyUserInfo->GetMonsterBookList() };
    if (true == ChkBook(_outPkt, monsterBookList, PktInfoMonsterBook::Piece::STATE::NEW_CHK, eBookGroup::Monster))
		return true;

    return false;
}

bool AutoPacketMsg::ChkAndGetUserSetMarkTID(uint32_t & _outTID) const
{
    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    c_size_t size{ userData.markInfos.size() };
    if (size <= 1)
		return false;
    _outTID             = userData.markID;

    while (userData.markID == _outTID)
        _outTID = userData.markInfos[ProjectMgr::ms_Inst->GetRand(0, size - 1)];
    return true;
}
bool AutoPacketMsg::ChkAndGetUserSetLobbyThemeTID(uint32_t & _outTID) const
{
    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    c_size_t size{ userData.lobbyThemes.size() };
    if (size <= 1)
		return false;
    _outTID             = userData.lobbyThemeID;

    while (userData.lobbyThemeID == _outTID)
        _outTID = userData.lobbyThemes[ProjectMgr::ms_Inst->GetRand(0, size - 1)];
    return true;
}
bool AutoPacketMsg::ChkAndGetUserSetMainCardFormation(uint32_t& _outID) const
{   
    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    uint32_t rand{};

    do {
        rand = (uint32_t)ProjectMgr::ms_Inst->GetRand(0, 2);
        _outID = rand;
    } while (rand == userData.cardFrmtID);
    
    return true;
}

bool AutoPacketMsg::ChkAndGetUserSetNamePkt(PktInfoStr & _outPkt) const
{
    STMaxLenFmtString nickStr{};
    nickStr.Format(_TX("%llu"), ProjectMgr::ms_Inst->GetRand(10000, 999999));
    if (0 == STRCMP(m_DummyUserInfo->GetNickName().c_str(), nickStr.Get()))
		return false;
    _outPkt.DoAdd(nickStr.Get());
    
    return true;
}

bool AutoPacketMsg::ChkAndGetUserSetCommentMsgPkt(PktInfoStr & _outPkt) const
{
    STMaxLenFmtString commStr{};
    commStr.Format(_TX("%llu"), ProjectMgr::ms_Inst->GetRand(1000000, 99999999));
    if (0 == STRCMP(m_DummyUserInfo->GetCommentMsg().c_str(), commStr.Get()))
		return false;
    _outPkt.DoAdd(commStr.Get());

    return true;
}
bool AutoPacketMsg::ChkAndGetUserSetCountryAndLangCode(PktInfoCountryLangCode& _outPkt) const
{
    PktInfoCountryLangCode& userLangCode{ m_DummyUserInfo->GetUserData().coLangCode };

    PktInfoCountryLangCode langCode{};
    langCode.country_.DoAdd(_TX("KOR"));
    langCode.lang_.DoAdd(_TX("KO"));

    // KOR 설정이면 
    if (userLangCode.country_.IsSame(langCode.country_))
    {
        _outPkt.country_.DoAdd(_TX("US"));
        _outPkt.lang_.DoAdd(_TX("JPN"));
    }
    else
    {
        _outPkt = langCode;
    }

    userLangCode = _outPkt;
    return true;
}

bool AutoPacketMsg::ChkBook(PktInfoBookNewConfirm& _pktInfo, const vecmap<BookData>& _bookList, c_uint32 _chkIdx, const eBookGroup _group) const
{
    c_size_t size{ _bookList.size() };
    for (size_t bookIdx{}; bookIdx < size; ++bookIdx)
    {
        if (false == _bookList.GetDataWithIdx(bookIdx).IsOnState(_chkIdx))
        {
            _pktInfo.bookTID_   = static_cast<uint32_t>(_bookList.GetKeyWithIdx(bookIdx));
            _pktInfo.bookGroup_ = _group;
            return true;
        }
    }
    return false;
}

bool AutoPacketMsg::CanAddItemSlot() const
{
    uint16_t itemSlotCnt{ m_DummyUserInfo->GetUserData().itemSlotCnt };
    uint64_t userGold{ m_DummyUserInfo->GetGoodsData()[eGOODSTYPE::GOLD] };

    const DataBalance& balData{ TableBalance::Inst->Get() };

    if (balData.AddItemSlotGold > userGold)
        return false;

    return itemSlotCnt < balData.MaxItemSlotCount;
}

bool AutoPacketMsg::ChkAndGetCardApplyPosPkt(PktInfoCardApplyPos & _outPkt) const
{
    const CARD_VECMAP& cardList{ m_DummyUserInfo->GetCardList() };
    const FACILITY_VECMAP& facilityList{ m_DummyUserInfo->GetFacilityList() };
    const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };

    c_size_t cardSize{ cardList.size() };
    c_size_t facilitySize{ facilityList.size() };
    c_size_t charSize{ charList.size() };
    if (0 == cardSize || (0 == facilitySize && 0 == charSize))
		return false;

    m_Idx.DoStart(cardSize - 1, _MAC_FUNC_);
	m_Idx.ChangeAndCheckEndIdx();
    do {
        const CardData cardData{ cardList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        if (cardData.posKind == eContentsPosKind::FACILITY)
			continue;

        _outPkt.cardUID_        = cardData.UID;
        if (facilitySize == 0)
            _outPkt.posKind_    = eContentsPosKind::CHAR;
        else if (charSize == 0)
            _outPkt.posKind_    = eContentsPosKind::FACILITY;
        else
            _outPkt.posKind_    = (eContentsPosKind)ProjectMgr::ms_Inst->GetRand(1, 2);

        if (_outPkt.posKind_ == eContentsPosKind::CHAR)
        {
            for (size_t charIdx = 0; charIdx < charSize; ++charIdx)
            {
				uint64_t cuid{ charList.GetDataWithIdx(charIdx).CUID };
                if(cuid != cardData.posValue)
                    _outPkt.posValue_ = cuid;
            }
        }
        else if (_outPkt.posKind_ == eContentsPosKind::FACILITY)
        {
            const FacilityData& facility{ facilityList.GetDataWithIdx(ProjectMgr::ms_Inst->GetRand(0, facilitySize - 1)) };
            if (0 == facility.operValue)
                _outPkt.posValue_   = facility.TID;
        }

        if (_outPkt.posValue_ != 0)
            return true;
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    return false;
}

bool AutoPacketMsg::ChkAndGetCardPosOutUID(uint64_t & _outUID) const
{
    const CARD_VECMAP& cardList{ m_DummyUserInfo->GetCardList() };
    c_size_t size{ cardList.size() };
    if (size == 0)
		return false;

    for (size_t cardIdx{}; cardIdx < size; ++cardIdx)
    {
        const CardData& card{ cardList.GetDataWithIdx(cardIdx) };

        if (card.posKind != eContentsPosKind::_NONE_)
        {
            if (card.posKind == eContentsPosKind::FACILITY)
            {
                const FacilityData& facility{ m_DummyUserInfo->GetFacilityList().GetDataWithIdx(card.posValue) };
                if (0 != facility.operValue)
                    continue;
            }

            _outUID     = cardList.GetDataWithIdx(cardIdx).UID;
            return true;
        }
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetCardSellPkt(PktInfoCardSell & _outPkt) const
{
    const CARD_VECMAP& cardList{ m_DummyUserInfo->GetCardList() };
    c_size_t size{ cardList.size() };
    if (size == 0)
		return false;

    for (size_t cardIdx{}; cardIdx < size; ++cardIdx)
    {
        const CardData& cardData    = cardList.GetDataWithIdx(cardIdx);
        if (true == cardData.CanUseCard())
        {
            PktInfoCardSell::Piece pktInfo{};
            pktInfo.cardUID_        = cardData.UID;
            _outPkt.infos_.push_back(pktInfo);
            return true;
        }
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetCardLockPkt(PktInfoCardLock & _outPkt) const
{
    const CARD_VECMAP& cardList{ m_DummyUserInfo->GetCardList() };
    if (cardList.size() == 0)
		return false;

    PktInfoCardLock::Piece pktInfo{};
    const CardData& cardData    = cardList.GetDataWithIdx(ProjectMgr::ms_Inst->GetRand(0, cardList.size() - 1));
    pktInfo.cardUID_            = cardData.UID;
    pktInfo.lock_               = cardData.lock ? false : true;

    _outPkt.infos_.push_back(pktInfo);

    return true;
}

bool AutoPacketMsg::ChkAndGetCardLvUpPkt(PktInfoProductComGrowReq & _outPkt) const
{
    const CARD_VECMAP& cardList{ m_DummyUserInfo->GetCardList() };
    const GOODS_ARR& goodsData{ m_DummyUserInfo->GetGoodsData() };
    if (cardList.size() == 0)
		return false;
    m_Idx.DoStart(cardList.size() - 1, _MAC_FUNC_);

    do {
        const CardData& cardData{ cardList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        if (_outPkt.targetUID_ == 0 && cardData.CanCardLvUp(goodsData[eGOODSTYPE::GOLD]))
            _outPkt.targetUID_      = cardData.UID;
        else if (_outPkt.targetUID_ != 0 && true == _outPkt.maters_.IsEmpty())
        {
            if (true == cardData.CanUseCard())
            {
                _outPkt.maters_.DoAdd(cardData.UID);
                return true;
            }
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    if (0 != _outPkt.targetUID_&& true == _outPkt.maters_.IsEmpty())
        return GetMaterItems(&_outPkt.materItems_, eITEMSUBTYPE::MATERIAL_CARD_LEVELUP, trueReturnWork);

    return false;
}

bool AutoPacketMsg::ChkAndGetCardSkillLvUpPkt(PktInfoProductComGrowReq & _outPkt) const
{
    const CARD_VECMAP& cardList{ m_DummyUserInfo->GetCardList() };
    c_size_t cardSize{ cardList.size() };
    if (0 >= cardSize)
		return false;
    m_Idx.DoStart(cardSize - 1, _MAC_FUNC_);
    uint64_t targetUID{};

    do {
        const CardData& cardData    = cardList.GetDataWithIdx(m_Idx.GetCurIdx());

        c_uint8 maxSkillLv{ TableBalance::Inst->Get().CardMaxSkillLevel };
        if (cardData.skillLv < maxSkillLv)
        {
            for (int idx{ 0 }; idx < cardSize; ++idx)
            {
                const CardData& tmpCard{ cardList.GetDataWithIdx(idx) };
                if (cardData.UID != tmpCard.UID && cardData.TID == tmpCard.TID && true == tmpCard.CanUseCard())
                {
                    _outPkt.targetUID_  = cardData.UID;
                    _outPkt.maters_.DoAdd(tmpCard.UID);
                    break;
                }
            }
        }
        targetUID                   = cardData.UID;
        if (0 != _outPkt.targetUID_)
			break;
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    if (true == _outPkt.maters_.IsEmpty())
    {
        _outPkt.targetUID_  = targetUID;
        return GetMaterItems(&_outPkt.materItems_, eITEMSUBTYPE::MATERIAL_CARD_SLVUP, trueReturnWork);
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetCardWakePkt(PktInfoProductComGrowReq & _outPkt) const
{
    const CARD_VECMAP& cardList{ m_DummyUserInfo->GetCardList() };
    c_size_t cardSize{ cardList.size() };
    if (cardSize <= 0)
        return false;

    m_Idx.DoStart(cardSize - 1, _MAC_FUNC_);

    do {
        const CardData& cardData    = cardList.GetDataWithIdx(m_Idx.GetCurIdx());
        if (0 == cardData.TID)
        {
			STMaxLenFmtString log{};
			log.Format(_TX("CardTid = 0, uid:%llu | UUID:%llu, curIdx:%llu, chkIdx:%llu, maxIdx:%llu, maxIdx:%llu"), cardData.UID, m_DummyUserInfo->GetUserData().UUID, m_Idx.GetCurIdx(), m_Idx.chkIdx, m_Idx.maxIdx, cardSize);

			ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_TST_);

            break;
        }
        c_uint32 wakeReqGroup{ TableCard::Inst->GetData(cardData.TID).WakeReqGroup };
        if (TableCard::Inst->GetCardWakeMax(cardData.grade) <= cardData.wake)
            continue;
        if (TableCard::Inst->GetMaxLvFromGradeWake(cardData.grade, cardData.wake) > cardData.lv)
            continue;

        const DataItemReqList& itemGroup{ GetItemReqData(wakeReqGroup, cardData.wake) };
        if (true == CheckItemGroup(itemGroup))
        {
            _outPkt.targetUID_      = cardData.UID;
            return true;
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    return false;
}

bool AutoPacketMsg::ChkAndGetCardFavorLvRewardPkt(PktInfoBookOnStateReq & _outPkt) const
{
    const BOOK_VECMAP& cardBookList{ m_DummyUserInfo->GetCardBookList() };
    if (0 >= cardBookList.size())
		return false;
    m_Idx.DoStart(cardBookList.size() - 1, _MAC_FUNC_);

    do {
        const BookData& cardBookData = cardBookList.GetDataWithIdx(m_Idx.GetCurIdx());
        if (1 <= cardBookData.lv)
            if (true == GetCardBookFavorState(cardBookData, _outPkt, PktInfoCardBook::Piece::FAVOR_RWD_GET_1))   return true;
        if (2 <= cardBookData.lv)
            if (true == GetCardBookFavorState(cardBookData, _outPkt, PktInfoCardBook::Piece::FAVOR_RWD_GET_2))   return true;
        if (3 <= cardBookData.lv)
            if (true == GetCardBookFavorState(cardBookData, _outPkt, PktInfoCardBook::Piece::FAVOR_RWD_GET_3))   return true;
		if (4 <= cardBookData.lv)
			if (true == GetCardBookFavorState(cardBookData, _outPkt, PktInfoCardBook::Piece::FAVOR_RWD_GET_4))   return true;
		if (5 <= cardBookData.lv)
			if (true == GetCardBookFavorState(cardBookData, _outPkt, PktInfoCardBook::Piece::FAVOR_RWD_GET_5))   return true;

    } while (false == m_Idx.ChangeAndCheckEndIdx());
    return false;
}
bool AutoPacketMsg::GetCardBookFavorState(const BookData& _cardBookData, PktInfoBookOnStateReq & _outPkt, PktInfoCardBook::Piece::STATE _stateType) const
{
    if (false == _cardBookData.IsOnState(_stateType))
    {
        _outPkt.tid_        = _cardBookData.TID;
        _outPkt.stateIdx_   = static_cast<uint8_t>(_stateType);
        return true;
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetItemSellPkt(PktInfoItemSell & _outPkt) const
{
    const ITEM_VECMAP& itemList{ m_DummyUserInfo->GetItemList() };
    if (0 >= itemList.size())
		return false;

    uint8_t itemIdx = static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, itemList.size() - 1));
    const ItemData& data    = itemList.GetDataWithIdx(itemIdx);
    if (data.count == 0)
		return false;

    uint32_t randCnt        = data.count;
    uint8_t itemCount{};

    if (data.count > 100)
        randCnt         = 100;

    if (data.count == 1) itemCount  = 1;
    else
        itemCount = static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(1, randCnt));

    if (itemCount != 0)
    {
        PktInfoItemSell::Piece sellItem{};
        sellItem.itemUID_   = data.UID;
        sellItem.sellCnt_   = itemCount;
        _outPkt.infos_.push_back(sellItem);
        return true;
    }

    return false;
}

bool AutoPacketMsg::ChkAndGetItemUsePkt(PktInfoUseItemReq& _outPkt) const
{
    const ITEM_VECMAP& itemList{ m_DummyUserInfo->GetItemList() };
    if (0 >= itemList.size()) 
        return false;

    PktInfoItemCntVec::Piece* findUseItem{ &_outPkt.item_ };

    const DataBalance& balData{ TableBalance::Inst->Get() };
    m_Idx.DoStart(itemList.size() - 1, _MAC_FUNC_);
    do 
    {
        const ItemData& itemData{ itemList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        findUseItem->uid_ = itemData.UID;
        findUseItem->cnt_ = 1;

        eITEMSUBTYPE type{ };
        if (false == itemData.ChkItemCanUseAndGetSubTp(&type))
			continue;

        switch (type)
        {
		case eITEMSUBTYPE::USE_AP_CHARGE:
		case eITEMSUBTYPE::USE_BP_CHARGE:
		{
			uint64_t nowTicket{ m_DummyUserInfo->GetGoodsData()[eGOODSTYPE::TICKET] };
			uint64_t addTicket{ TableLevelUp::Inst->GetData(balData.AccountLevelUpGroup, m_DummyUserInfo->GetUserData().expLv.lv_).Value1 };
			uint64_t maxVal{ balData.LimitMaxAP };
			if (eITEMSUBTYPE::USE_BP_CHARGE == type)
			{
				nowTicket = m_DummyUserInfo->GetGoodsData()[eGOODSTYPE::BATTLETICKET];
				addTicket = itemData.GetVal();
				maxVal = balData.LimitMaxBP;
			}
			if (nowTicket + addTicket <= maxVal)
				return true;
		} break;
		case eITEMSUBTYPE::USE_SELECT_ITEM:
		{
			const DataRandom& dataRand{ TableRandom::Inst->DoRandGet(itemData.GetVal()) };
			if (true == dataRand.IsNull())
				continue;

			_outPkt.value1_ = dataRand.Value;
			return true;

		} break;
		case eITEMSUBTYPE::USE_PROMOTION_ITEM: { return true; } break;
        default:        continue;
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    return false;
}

bool AutoPacketMsg::ChkAndGetGemSellPkt(PktInfoGemSell & _outPkt) const
{
    const GEM_VECMAP& gemList{ m_DummyUserInfo->GetGemList() };
    PktInfoGemSell::Piece pktInfo{};
    c_size_t size{ gemList.size() };
    if (size == 0)
		return false;

    for (size_t gemIdx{}; gemIdx < size; ++gemIdx)
    {
        const GemData& gemData  = gemList.GetDataWithIdx(gemIdx);
        if (true == gemData.CanUseGem())
        {
            pktInfo.gemUID_     = gemData.gemUID;
            _outPkt.infos_.push_back(pktInfo);
            return true;
        }
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetGemLockPkt(PktInfoGemLock & _outPkt) const
{
    const GEM_VECMAP& gemList{ m_DummyUserInfo->GetGemList() };
    if (gemList.size() == 0)
		return false;

    PktInfoGemLock::Piece pktInfo;
    const GemData& gemData  = gemList.GetDataWithIdx(static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, gemList.size() - 1)));
    pktInfo.gemUID_         = gemData.gemUID;
    pktInfo.lock_           = gemData.lock ? false : true;

    _outPkt.infos_.push_back(pktInfo);
    return true;
}

bool AutoPacketMsg::ChkAndGetGemResetOptPkt(PktInfoGemResetOptReq & _outPkt) const
{
    const GEM_VECMAP& gemList{ m_DummyUserInfo->GetGemList() };
    c_size_t size{ gemList.size() };
    if (0 >= size)
		return false;

    m_Idx.DoStart(size - 1, _MAC_FUNC_);
    do {
        const GemData& gemData{ gemList.GetDataWithIdx(m_Idx.GetCurIdx()) };

        if(0 == gemData.gemUID) 
            break;

        if (0 != gemData.wake && false  == gemData.resetOpt)
        {
            const DataItemReqList& itemGroup{ GetItemReqData(gemData.GetOptResetReqGroup()) };
            if (true == CheckItemGroup(itemGroup))
            {
                _outPkt.gemUID_     = gemData.gemUID;
                _outPkt.slotIdx_    = static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, gemData.wake - 1));
                return true;
            }
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());
    return false;
}

bool AutoPacketMsg::ChkAndGetGemResetOptSelectPkt(PktInfoGemResetOptSelect & _outPkt) const
{
    const GEM_VECMAP& gemList{ m_DummyUserInfo->GetGemList() };
    c_size_t size{ gemList.size() };
    if (0 >= size)
		return false;

    m_Idx.DoStart(size - 1, _MAC_FUNC_);
    do {
        const GemData& gemData  = gemList.GetDataWithIdx(m_Idx.GetCurIdx());
        if (true == gemData.resetOpt)
        {
            _outPkt.gemUID_     = gemData.gemUID;
            _outPkt.newFlag_    = (0 == ProjectMgr::ms_Inst->GetRand(0, 1)) ? true : false;
            return true;
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());
    return false;
}

bool AutoPacketMsg::ChkAndGetLvUpGemPkt(PktInfoProductComGrowReq & _outPkt) const
{
    const GEM_VECMAP& gemList{ m_DummyUserInfo->GetGemList() };
    c_size_t size{ gemList.size() };
    if (0 >= size)
		return false;

    m_Idx.DoStart(size - 1, _MAC_FUNC_);
    do {
        const GemData& gemData  = gemList.GetDataWithIdx(m_Idx.GetCurIdx());
        if (_outPkt.targetUID_ != 0)
        {
            if (true == gemData.CanUseGem())
            {
                _outPkt.maters_.DoAdd(gemData.gemUID);
                return true;
            }
        }
		else if (0 < gemData.GetCanUpGemLv())
		{
			uint64_t userGold{ m_DummyUserInfo->GetGoodsData()[eGOODSTYPE::GOLD] };
			if (TableGem::Inst->GetGemLevelupCostByGrade(gemData.lv) <= userGold)
				_outPkt.targetUID_  = gemData.gemUID;
		}
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    return false;
}

bool AutoPacketMsg::ChkAndGetWakeGemPkt(PktInfoProductComGrowReq & _outPkt) const
{
    const GEM_VECMAP& gemList{ m_DummyUserInfo->GetGemList() };

    c_size_t size{ gemList.size() };
    if (0 >= size)
		return false;
    for (int gemIdx{ 0 }; gemIdx < size; ++gemIdx)
    {
        const GemData& gemData      = gemList.GetDataWithIdx(gemIdx);
        if (true == gemData.CanWakeGem())
        {
            const DataItemReqList& itemGroup{ GetItemReqData(gemData.GetWakeReqGroup(), gemData.wake) };
            if (true == CheckItemGroup(itemGroup))
            {
                _outPkt.targetUID_  = gemData.gemUID;
                return true;
            }
        }
    }

    return false;
}

bool AutoPacketMsg::ChkAndGetSellWeaponPkt(PktInfoWeaponSell& _outPkt) const
{
    const WEAPON_VECMAP& weaponList{ m_DummyUserInfo->GetWeaponList() };
    PktInfoWeaponSell::Piece pktInfo{};

    c_size_t size{ weaponList.size() };
    for (int weaponIdx{ 0 }; weaponIdx < size; ++weaponIdx)
    {
        const WeaponData& weaponData{ weaponList.GetDataWithIdx(weaponIdx) };
        if (weaponData.CanUseWeapon() && CheckAlreadyFacilityOper(weaponData.UID))
        {
            pktInfo.weaponUID_  = weaponData.UID;
            _outPkt.infos_.push_back(pktInfo);
            return true;
        }
        ++weaponIdx;
    }

    return false;
}

bool AutoPacketMsg::ChkAndGetWeaponLockPkt(PktInfoWeaponLock & _outPkt) const
{
    const WEAPON_VECMAP& weaponList{ m_DummyUserInfo->GetWeaponList() };
    c_size_t size{ weaponList.size() };
    if (0 >= size)
		return false;

    PktInfoWeaponLock::Piece pktInfo{};
    const WeaponData& weaponData    = weaponList.GetDataWithIdx(static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, weaponList.size() - 1)));
    pktInfo.weaponUID_              = weaponData.UID;
    pktInfo.lock_                   = weaponData.lock ? false : true;

    _outPkt.infos_.push_back(pktInfo);

    return true;
}

bool AutoPacketMsg::ChkAndGetWeaponLvUpPkt(PktInfoProductComGrowReq & _outPkt) const
{
    const WEAPON_VECMAP& weaponList{ m_DummyUserInfo->GetWeaponList() };
    const GOODS_ARR& goodsData{ m_DummyUserInfo->GetGoodsData() };
    c_size_t size{ weaponList.size() };
    if (0 >= size)
		return false;

    m_Idx.DoStart(size - 1, _MAC_FUNC_);
    do {
        const WeaponData& weaponData{ weaponList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        if (_outPkt.targetUID_ != 0)
        {
            if (weaponData.CanUseWeapon() && CheckAlreadyFacilityOper(weaponData.UID))
            {
                _outPkt.maters_.DoAdd(weaponData.UID);
                return true;
            }
        }
        else if (weaponData.CanWeaponLvUp(goodsData[eGOODSTYPE::GOLD]) && CheckAlreadyFacilityOper(weaponData.UID))
            _outPkt.targetUID_  = weaponData.UID;
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    if (_outPkt.targetUID_ != 0 && true == _outPkt.maters_.IsEmpty())
        return GetMaterItems(&_outPkt.materItems_, eITEMSUBTYPE::MATERIAL_WEAPON_LEVELUP, trueReturnWork);

    return false;
}

bool AutoPacketMsg::ChkAndGetWeaponSkillLvUpPkt(PktInfoProductComGrowReq & _outPkt) const
{
    const WEAPON_VECMAP& weaponList{ m_DummyUserInfo->GetWeaponList() };
    c_size_t weaponSize{ weaponList.size() };

    if (weaponSize <= 0)
		return false;
    m_Idx.DoStart(weaponSize - 1, _MAC_FUNC_);

    WeaponData targetData{};
    do {
        const WeaponData& weaponData    = weaponList.GetDataWithIdx(m_Idx.GetCurIdx());

        c_uint8 maxSkillLv{ TableBalance::Inst->Get().WeaponMaxSkillLevel };
        if (weaponData.skillLv < maxSkillLv)
        {
            for (int idx{ 0 }; idx < weaponSize; ++idx)
            {
                const WeaponData& tmpWeapon{ weaponList.GetDataWithIdx(idx) };
                if (weaponData.UID != tmpWeapon.UID && weaponData.TID == tmpWeapon.TID
                    && true == tmpWeapon.CanUseWeapon() && CheckAlreadyFacilityOper(tmpWeapon.UID))
                {
                    _outPkt.targetUID_ = weaponData.UID;
                    _outPkt.maters_.DoAdd(tmpWeapon.UID);
                    break;
                }
            }
        }
        targetData      = weaponData;
        if (_outPkt.targetUID_ != 0)
			break;
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    if (true == _outPkt.maters_.IsEmpty())
    {
        _outPkt.targetUID_  = targetData.UID;
        auto work           = [&targetData](const DataItem& itemData) { return targetData.GetWeaponGrade() - 1 <= itemData.Grade; };
        return GetMaterItems(&_outPkt.materItems_, eITEMSUBTYPE::MATERIAL_WEAPON_SLVUP, work);
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetWeaponWakePkt(PktInfoProductComGrowReq & _outPkt) const
{
    const WEAPON_VECMAP& weaponList{ m_DummyUserInfo->GetWeaponList() };

    c_size_t weaponSize{ weaponList.size() };
    for (int weaponIdx{ 0 }; weaponIdx < weaponSize; ++weaponIdx)
    {
        const WeaponData& weaponData    = weaponList.GetDataWithIdx(weaponIdx);
        if (true == weaponData.CanWeaponWake())
        {
            const DataItemReqList& itemGroup{ GetItemReqData(weaponData.GetWakeReqGroup(), weaponData.wake) };
            if (true == CheckAlreadyFacilityOper(weaponData.UID) && true == CheckItemGroup(itemGroup))
            {
                _outPkt.targetUID_      = weaponData.UID;
                return true;
            }
        }
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetWeaponSlotGemPkt(PktInfoWeaponSlotGem & _outPkt) const
{
    const WEAPON_VECMAP& weaponList{ m_DummyUserInfo->GetWeaponList() };
    const GEM_VECMAP& gemList{ m_DummyUserInfo->GetGemList() };
    c_size_t weaponSize{ weaponList.size() };

    if (weaponSize <= 0)
		return false;
    m_Idx.DoStart(weaponSize - 1, _MAC_FUNC_);
    m_Idx.ChangeAndCheckEndIdx();

    const WeaponData& weaponData{ weaponList.GetDataWithIdx(m_Idx.GetCurIdx()) };
    for (uint8_t slotIdx = 0; slotIdx < weaponData.GetWeaponGemSlot(); ++slotIdx)
    {
        c_size_t gemSize{ gemList.size() };
        uint64_t gemUID = 0;
        for (size_t gemIdx{}; gemIdx < gemSize; ++gemIdx)
        {
            const GemData& gemData  = gemList.GetDataWithIdx(gemIdx);
            if (gemData.equip == false)
                gemUID  = gemData.gemUID;

            for (uint8_t i = 0; i < PktInfoWeapon::Slot::_MAX_; ++i)
            {
                if (_outPkt.gemUIDs_[i] == gemUID)
                    gemUID          = 0;
            }

            if (gemUID != 0)
            {
                _outPkt.weaponUID_  = weaponData.UID;
                break;
            }
        }
        _outPkt.gemUIDs_[slotIdx]   = gemUID;
    }

    return _outPkt.weaponUID_ != 0;
}
bool AutoPacketMsg::ChkAndGetWpnDepotAddSlot(uint8_t& _addCnt) const
{
    const PktInfoWpnDepotSet::Value& wpnDepotInfo{ m_DummyUserInfo->GetWpnDepotValue() };
    const BOOK_VECMAP& weaponBookList{ m_DummyUserInfo->GetWeaponBookList() };

    c_size_t bookCnt{ weaponBookList.size() };
    //uint8_t randCnt = static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(1, 2));
    c_uint32 maxSlotCnt{ static_cast<c_uint32>(wpnDepotInfo.maxCnt_) };
    
    const ITEM_VECMAP& itemList{ m_DummyUserInfo->GetItemList() };

    c_size_t itemSize{ itemList.size() };
    if (itemSize <= 0)
        return false;


    const DataItemReqList& itemReqData{ 
        TableItemReqList::Inst->GetData(TableBalance::Inst->Get().WpnDepotItemReqGroup, wpnDepotInfo.maxCnt_ + 1) };

    //무기고 슬롯 확장 시 GameConfig_Balance.WpnDepotItemReqGroup(9000(에 해당하는 Game_ItemReqList 테이블 참조
    if (true == itemReqData.IsNull())
        return false;

    uint64_t userCash{ m_DummyUserInfo->GetGoodsData()[eGOODSTYPE::CASH] };
    if (CheckItemGroup(itemReqData, static_cast<uint32_t>(bookCnt), static_cast<uint32_t>(userCash)))
    {
        // 우선은 자동에선 1번만 추가 시도함
        _addCnt = 1;
        return true;
    }
    
    return false;
}
bool AutoPacketMsg::ChkAndGetWpnDepotApplyWeapon(PktInfoWpnDepotApply& _outPklt) const
{
    // 서버에서는 그냥 덮어씌우기인데 순서가 바뀔경우는 어떻게하지..
    //const WPNDEPOT_VECMAP& wpnDepotList{ m_DummyUserInfo->GetWpnDepotList() };
    const PktInfoWpnDepotSet::Value& wpnDepotInfo{ m_DummyUserInfo->GetWpnDepotValue() };
    const WEAPON_VECMAP& userWeaponList{ m_DummyUserInfo->GetWeaponList() };
        
    c_size_t wpnDepotMaxCnt{ wpnDepotInfo.maxCnt_ };
    c_size_t userWeaponListSize{ userWeaponList.size() };

    if (1 > wpnDepotMaxCnt)
        return false;

    if (wpnDepotMaxCnt > userWeaponListSize)
        return false;

    // 최소 1개부터 랜덤으로 무기고 슬롯 최대수보다 작은수만큼 랜덤으로
    uint32_t randCnt{ static_cast<uint32_t>(ProjectMgr::ms_Inst->GetRand(1, wpnDepotMaxCnt)) };
    PktInfoTIDList tmpTIDs{ };

    m_Idx.DoStart(userWeaponListSize - 1, _MAC_FUNC_);
    do {
        const WeaponData& weaponData{ userWeaponList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        
        // 목록중에 잘못된 데이터가 있을수도있으니 다음 데이터로
        if (INVALID_UID == weaponData.UID)
            continue;

        if (weaponData.CanUseWeapon() && CheckAlreadyFacilityOper(weaponData.UID)) {
            
            // 이미 등록된 TID라면 패스
            if (tmpTIDs.IsExistSameTID(weaponData.TID))
                continue;

            tmpTIDs.DoAdd(weaponData.TID);
            _outPklt.slots_.push_back(PktInfoWpnDepotSet::Piece{ weaponData.UID });

            // 전달할 무기수가 임의의 크기가 되면 그만
            if (_outPklt.slots_.size() >= randCnt)
                return true;
        }

    } while (false == m_Idx.ChangeAndCheckEndIdx());
    
    return false;
}

bool AutoPacketMsg::ChkAndGetBadgeApplyPosPkt(PktInfoBadgeApplyPos& _outPkt) const
{
    const BADGE_VECMAP& badgeList{ m_DummyUserInfo->GetBadgeList() };
    size_t size{ badgeList.size() };
	if (size <= 0)
		return false;
    m_Idx.DoStart(size - 1, _MAC_FUNC_);
    do {
        const BadgeData& badgeData{ badgeList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        if (badgeData.posKind == eContentsPosKind::_NONE_)
        {
			_outPkt.badgeUID_	= badgeData.UID;
			_outPkt.posValue_	= m_DummyUserInfo->GetUserData().UUID;
            _outPkt.posKind_    = eContentsPosKind::ARENA;
            _outPkt.slotNum_    = static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, eBadgeSlotPosMax::ARENA - 1));
            return true;
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    return false;
}

bool AutoPacketMsg::ChkAndGetBadgeOutPosPkt(PktInfoBadgeComReq& _outPkt) const
{
    const BADGE_VECMAP& badgeList{ m_DummyUserInfo->GetBadgeList() };
    size_t size{ badgeList.size() };
	if (size <= 0) 
		return false;
    m_Idx.DoStart(size - 1, _MAC_FUNC_);
    do {
        const BadgeData& badgeData{ badgeList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        if (badgeData.posKind != eContentsPosKind::_NONE_)
        {
            _outPkt.badgeUID_   = badgeData.UID;
            return true;
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    return false;
}

bool AutoPacketMsg::ChkAndGetBadgeSellPkt(PktInfoBadgeSell& _outPkt) const
{
    const BADGE_VECMAP& badgeList{ m_DummyUserInfo->GetBadgeList() };
    size_t size{ badgeList.size() };
	if (size <= 0)
		return false;
    m_Idx.DoStart(size - 1, _MAC_FUNC_);
    do {
        const BadgeData& badgeData{ badgeList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        if (true == badgeData.CanUseBadge())
        {
            PktInfoBadgeSell::Piece data{};
            data.badgeUID_  = badgeData.UID;
            _outPkt.infos_.push_back(data);
            return true;
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());

    return false;
}

bool AutoPacketMsg::ChkAndGetBadgeLockPkt(PktInfoBadgeLock& _outPkt) const
{
    const BADGE_VECMAP& badgeList{ m_DummyUserInfo->GetBadgeList() };
	size_t size{ badgeList.size() };
    if (size <= 0)
		return false;

    PktInfoBadgeLock::Piece pktInfo{};
    const BadgeData& badgeData = badgeList.GetDataWithIdx(ProjectMgr::ms_Inst->GetRand(0, size - 1));
    pktInfo.badgeUID_   = badgeData.UID;
    pktInfo.lock_       = badgeData.lock ? false : true;

    _outPkt.infos_.push_back(pktInfo);
    return true;
}

bool AutoPacketMsg::ChkAndGetBadgeUpgradePkt(PktInfoBadgeComReq& _outPkt) const
{
    const BADGE_VECMAP& badgeList{ m_DummyUserInfo->GetBadgeList() };
    const TableBadgeOpt& tblBadgeOpt{ *TableBadgeOpt::Inst };
    uint64_t userGold{ m_DummyUserInfo->GetGoodsData()[eGOODSTYPE::GOLD] };

    size_t size{ badgeList.size() };
	if (size <= 0)
		return false;
    m_Idx.DoStart(size - 1, _MAC_FUNC_);
    do {
        const BadgeData& badgeData{ badgeList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        uint64_t lvUpCost = tblBadgeOpt.GetBadgeLvUpCostByLv(badgeData.lv);
        c_uint8 costItemCnt{ tblBadgeOpt.GetBadgeLvUpMatCntByLv(badgeData.lv) };
        
        if(true == ChkCanUseItemByTIDAndGetUID(TableItem::Inst->GetBadgeUpgradeMaterTID(), costItemCnt)
            && lvUpCost < userGold)
        {
            _outPkt.badgeUID_ = badgeData.UID;
            return true;
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());
    return false;
}

bool AutoPacketMsg::ChkAndGetBadgeResetUpgradePkt(PktInfoBadgeComReq& _outPkt) const
{
    const BADGE_VECMAP& badgeList{ m_DummyUserInfo->GetBadgeList() };
    uint64_t userGold{ m_DummyUserInfo->GetGoodsData()[eGOODSTYPE::GOLD] };

    size_t size{ badgeList.size() };
	if (size <= 0) 
		return false;
    m_Idx.DoStart(size - 1, _MAC_FUNC_);
    do {
        const BadgeData& badgeData{ badgeList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        uint64_t lvUpCost       = TableBalance::Inst->Get().BadgeLvInitCost;

        if (true == ChkCanUseItemByTIDAndGetUID(TableItem::Inst->GetBadgeResetUpgradeMaterTID(), 1)
            && lvUpCost < userGold)
        {
            _outPkt.badgeUID_   = badgeData.UID;
            return true;
        }
    } while (false == m_Idx.ChangeAndCheckEndIdx());
    return false;
}

bool AutoPacketMsg::ChkAndGetUpgradeFacilityTID(uint32_t & _outTID) const
{
    const FACILITY_VECMAP& facilityList{ m_DummyUserInfo->GetFacilityList() };

    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    auto _work          = [this, &facilityList, &userData, &_outTID](c_uint64, const DataDefault& _data)
    {
                        const DataFacility& facilityData{ static_cast<const DataFacility&>(_data) };

                        if (facilityData.FacilityOpenUserRank <= userData.expLv.lv_)
                        {
                            uint8_t facilityLv  = facilityList.GetDataWithKey(facilityData.ID).lv;

                            if (facilityLv < facilityData.MaxLevel)
                            {
                                const DataItemReqList& itemReqData = GetItemReqData(facilityData.LevelUpItemReq, facilityLv);
                                if (!itemReqData.IsNull() && CheckItemGroup(itemReqData, facilityLv))
                                {
                                    _outTID     = facilityData.ID;
                                    return true;
                                }
                            }
                        }
                        return false;
                    };
    return TableFacility::Inst->DoWorkToDatas(_work);
}

bool AutoPacketMsg::ChkAndGetFacilityOperationPkt(PktInfoFacilityOperationReq & _outPkt) const
{
    const FACILITY_VECMAP& facilityList{ m_DummyUserInfo->GetFacilityList() };
    c_size_t facilitySize{ facilityList.size() };
    if (0 >= facilitySize)
		return false;
    for (uint8_t facilityIdx = 0; facilityIdx < facilitySize; ++facilityIdx)
    {
        const FacilityData& data = facilityList.GetDataWithIdx(facilityIdx);
        if (false == data.GetCanFacilityOper())
			continue;

        if (data.operType == static_cast<uint8_t>(eFacilityEffTP::FAC_CHAR_EXP) || data.operType == static_cast<uint8_t>(eFacilityEffTP::FAC_CHAR_SP))
        {
            const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
            c_size_t charSize{ charList.size() };
            for (uint8_t charIdx = 0; charIdx < charSize; ++charIdx)
            {
                const CharData& charData{ charList.GetDataWithIdx(charIdx) };
                if ((data.operType == static_cast<uint8_t>(eFacilityEffTP::FAC_CHAR_EXP) && charData.GetCanLvUpChar())
                    || (data.operType == static_cast<uint8_t>(eFacilityEffTP::FAC_CHAR_SP) && charData.GetCanUpCharSP()))
                {
                    if (true == CheckAlreadyFacilityOper(charData.CUID))
                    {
                        _outPkt         = PktInfoFacilityOperationReq{ charData.CUID, data.TID, 1 };
                        return true;
                    }
                }
            }
        }
        else if (data.operType == static_cast<uint8_t>(eFacilityEffTP::FAC_WEAPON_EXP))
        {
            const WEAPON_VECMAP& weaponList{ m_DummyUserInfo->GetWeaponList() };
            c_size_t weaponSize{ weaponList.size() };
            for (uint8_t weaponIdx = 0; weaponIdx < weaponSize; ++weaponIdx)
            {
                const WeaponData& weaponData{ weaponList.GetDataWithIdx(weaponIdx) };
                if (weaponData.ChkCanWeaponInFacility() && CheckAlreadyFacilityOper(weaponData.UID)) {
                    _outPkt             = PktInfoFacilityOperationReq{ weaponData.UID, data.TID, 1 };
                    return true;
                }
            }
        }
        else if (data.operType == static_cast<uint8_t>(eFacilityEffTP::FAC_ITEM_COMBINE))
        {
            auto _work  = [this, &_outPkt, &data](c_uint64, const DataDefault& _data)
            {
                        const DataFacilityCombine* combineData = static_cast<const DataFacilityCombine*>(&_data);

                        if (data.lv < combineData->ReqFacilityLv)
							return false;

                        const DataItemReqList& itemReqData = GetItemReqData(combineData->ItemReqGroup);
                        if (true == CheckItemGroup(itemReqData, data.lv))
                        {
                            _outPkt     = PktInfoFacilityOperationReq{ combineData->ID, data.TID,  1 };
                            return true;
                        }
                        return false;
                    };
            return TableFacilityItemCombine::Inst->DoWorkToDatas(_work);
        }
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetFacilityOperConfirmPkt(PktInfoFacilityOperConfirmReq & _outPkt) const
{
    const FACILITY_VECMAP& facilityList{ m_DummyUserInfo->GetFacilityList() };
    c_size_t facilitySize{ facilityList.size() };
    if (0 >= facilitySize)
		return false;
    vector<int64_t> gapTimeList_sec{};
    for (uint8_t facilityIdx = 0; facilityIdx < facilityList.size(); ++facilityIdx)
    {
        const FacilityData& data    = facilityList.GetDataWithIdx(facilityIdx);
        datetime_num nowTime{ ProjectMgr::ms_Inst->GetNowDateTimeNum() };
        datetime_num operTime{ data.operEndTime.time_ };
        if (data.operEndTime.time_ != 0 && operTime <= nowTime)
        {
            _outPkt     = PktInfoFacilityOperConfirmReq{ data.TID, true };
            return true;
        }
        if (0 == operTime)  gapTimeList_sec.push_back(0);
        else    gapTimeList_sec.push_back(ProjectMgr::ms_Inst->DoCalcDateTimeToTime_Ms(nowTime, operTime) / 1000);
    }

    uint8_t minGapTimeIdx{};
    c_size_t gapSize{ gapTimeList_sec.size() };
    for (uint8_t tmpIdx{}; tmpIdx < gapTimeList_sec.size(); ++tmpIdx)
    {
        if (0 == gapTimeList_sec[tmpIdx])
			continue;

        if (0 == gapTimeList_sec[minGapTimeIdx] || gapTimeList_sec[minGapTimeIdx] > gapTimeList_sec[tmpIdx])
			minGapTimeIdx = tmpIdx;
    }

    if (0 == gapTimeList_sec[minGapTimeIdx])
        return false;

    const ITEM_VECMAP& itemList{ m_DummyUserInfo->GetItemList() };
    c_size_t itemSize{ itemList.size() };
    for (uint16_t itemIdx{}; itemIdx < itemSize; ++itemIdx)
    {
        const ItemData& itemData{ itemList.GetDataWithIdx(itemIdx) };
        int32_t itemTimeVal{ itemData.ChkItemTypeAndGetVal(eITEMTYPE::MATERIAL, eITEMSUBTYPE::MATERIAL_FACILITY_TIME) };
        if (0 < itemTimeVal)
        {
            double itemCnt_dbl{ static_cast<double>(gapTimeList_sec[minGapTimeIdx]) / (itemTimeVal * 60) };
            c_uint32 itemCnt{ static_cast<c_uint32>(ceil(itemCnt_dbl)) };

            if (itemData.count >= itemCnt)
            {
                _outPkt = PktInfoFacilityOperConfirmReq{ facilityList.GetDataWithIdx(minGapTimeIdx).TID, true, itemData.TID };
                return true;
            }
        }
    }
     return false;
}

bool AutoPacketMsg::ChkAndGetDispatchOpenPkt(PktInfoTIDList& _outPkt) const
{
    // Game_CardDispatchSlot.NeedRank 이상의 지휘관 랭크 필요
    // Game_CardDispatchSlot.OpenGoods 에 해당하는 재화를 Game_CardDispatchSlot.OpenValue 만큼 소모
    // 같은 서포터가 있을시에는 추가 안되게
    // 개방시 첫파견의 등급은 Game_CardDispatchSlot.InitGrade 에 해당하는 파견등급에서 랜덤    

    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    c_uint8 nowRank{ userData.expLv.lv_ };

    const DISPATCH_VECMAP& userDispatchList{ m_DummyUserInfo->GetDispatchList()};
    c_size_t dispatchSize{ userDispatchList.size() };

    if (DataCardDispatchSlot::Rate::_MAX_ - 1 <= dispatchSize)
        return false;

    const TableCardDispatchSlot& tbl{ *TableCardDispatchSlot::Inst };
    const GOODS_ARR& goodsData{ m_DummyUserInfo->GetGoodsData() };
        
    const DataCardDispatchSlot& dataSlot{ tbl.GetData(dispatchSize + 1) };

    if (true == dataSlot.IsNull())
        return false;

    if (dataSlot.NeedRank > nowRank)
        return false;

    if (dataSlot.OpenValue > goodsData[dataSlot.OpenGoods])
        return false;

    const DataCardDispatchMission& dataMission{ dataSlot.GetInitRandMission() };

    if (true == dataMission.IsNull())
        return false;

    _outPkt.DoAdd(static_cast<c_uint32>(dataSlot.Index));

    return true;
}

bool AutoPacketMsg::ChkAndGetDispatchChangePkt(PktInfoTIDList& _outPkt) const
{
    // 파견이 진행중이지 않은 슬롯은 CardDispatchSlot.ChangeGoods 에 해당하는 재화를 ChangeValue만큼 소모 하여 교체
    // 교체시 DispatchSlot.GradeRate1 ~ GradeRate5 파견 등급 상대 확률에 따라 해당등급의 파견등급중에서 랜덤
    // 해당 슬롯 인덱스가 아니고 오픈한 최대 슬롯 인덱스로 공통
    // 교체시 배포되었던 서포터 일괄 해제

    const DISPATCH_VECMAP& userDispatchList{ m_DummyUserInfo->GetDispatchList() };
    c_size_t dispatchSize{ userDispatchList.size() };

    if (1 > dispatchSize)
        return false;

    const TableCardDispatchSlot& tbl{ *TableCardDispatchSlot::Inst };
    const GOODS_ARR& goodsData{ m_DummyUserInfo->GetGoodsData() };

    // 1 ~ dispatchSlotMAX 중 임의의값
    uint64_t rand = ProjectMgr::ms_Inst->GetRand(1, dispatchSize);
    
    for (uint32_t idx{ }; idx < rand; ++idx)
    {
        const DispatchData dispatchData{ userDispatchList.GetDataWithIdx(idx) };

        if (false == dispatchData.operEndTime.IsZero())
            return false;

        // 테이블은 1번인덱스부터 시작 한다.
        const DataCardDispatchSlot& dataSlot{ tbl.GetData(idx + 1) };

        if (true == dataSlot.IsNull())
            return false;

        if (dataSlot.ChangeValue > goodsData[dataSlot.ChangeGoods])
            return false;

        _outPkt.DoAdd(dispatchData.TID);
    }
    
    return true;
}
bool AutoPacketMsg::ChkAndGetDispatchOperationPkt(PktInfoDispatchOperReq& _outPkt) const
{
    // 배치할 서포터 소켓 타입(Game_CardDispatchMission.SocketType1~SocketType5)에 해당하는 서포터만 리스트에 출력함
    // 현재 서포터 소켓이 아닌 이미 장착(캐릭터, 시설, 파견 슬롯 모두 포함) 중인 서포터는 리스트에 출력하지 않음
    // 현재 서포터 소켓에 배치된 서포터는 리스트 가장 앞에 출력, 동일 서포터 선택 시 해제 처리
    // 해당 파견 슬롯에 배치된 서포터와 동일한 TID의 서포터는 리스트에 출력하지 않음
    const DISPATCH_VECMAP& userDispatchList{ m_DummyUserInfo->GetDispatchList() };
    const CARD_VECMAP& userCardList{ m_DummyUserInfo->GetCardList() };
    c_size_t dispatchSize{ userDispatchList.size() };
    c_size_t cardSize{ userCardList.size() };

    CARD_VECMAP operCardList{};
    
    const TableCardDispatchSlot& tbl{ *TableCardDispatchSlot::Inst };
    const TableCard& cardTbl{ *TableCard::Inst };

    PktInfoDispatch::Piece dispatchData{};
    
    // 파견중이 아닌 슬롯을 찾음
    for (uint32_t idx{ }; idx < dispatchSize; ++idx)
    {
        const DispatchData& tempDispatchData{ userDispatchList.GetDataWithIdx(idx) };

        if (tempDispatchData.operEndTime.IsZero())
        {
            dispatchData.tableID_ = tempDispatchData.TID;
            dispatchData.missionID_ = tempDispatchData.MID;
            break;
        }
    }
    
    // 검색된 파견정보 테이블이 없으면 false
    if (0 == dispatchData.tableID_)
        return false;

    const DataCardDispatchSlot& dataSlot{ tbl.GetData(dispatchData.tableID_) };
    if (true == dataSlot.IsNull())
        return false;

    const DataCardDispatchMission& dataDispatchMission{ dispatchData.MissionTable(_MAC_FUNC_) };
    if (true == dataDispatchMission.IsNull())
        return false;

    
    uint64_t applyCardCnt{};
    uint64_t applyCardURCnt{};
    std::vector<uint64_t> cardUIDs{};
    PktInfoTIDList checkTIDList;

    c_size_t cardCnt{ static_cast<c_size_t>(dataDispatchMission.needCardCnt - dataDispatchMission.NeedURCnt) };
    // 파견중이 아닌 카드를 찾음
    m_Idx.DoStart(cardSize - 1, _MAC_FUNC_);
    do {

        const CardData& cardData{ userCardList.GetDataWithIdx(m_Idx.GetCurIdx()) };
        const DataCard& dataCard{ cardTbl.GetData(cardData.TID) };
        
        //사용중이면 continue
        if (eContentsPosKind::_NONE_ != cardData.posKind)
            continue;

        // 슬롯에 적용되어있으면 continue
        if (0 != cardData.posSlotNum)
            continue;

        // 이미 적용중인 종류의 카드라면 continue
        if (checkTIDList.IsExistSameTID(cardData.TID))
            continue;
        
        // UR등급의 카드가 필요할경우
        if (4 <= cardData.grade && dataDispatchMission.NeedURCnt > applyCardURCnt)
        {
            if (eSTAGE_CONDI::NOT_CHECK_CONDI != static_cast<eSTAGE_CONDI>(dataDispatchMission.SlotType[applyCardCnt])
                && dataDispatchMission.SlotType[applyCardCnt] != dataCard.Type)
                continue;

            cardUIDs.push_back(cardData.UID);
            checkTIDList.DoAdd(cardData.TID);
            ++applyCardCnt;
            ++applyCardURCnt;
        }
        // UR 등급제외 일반카드 
        else if (cardCnt > cardUIDs.size())
        {
            if (eSTAGE_CONDI::NOT_CHECK_CONDI != static_cast<eSTAGE_CONDI>(dataDispatchMission.SlotType[applyCardCnt])
                && dataDispatchMission.SlotType[applyCardCnt] != dataCard.Type)
                continue;
            
            cardUIDs.push_back(cardData.UID);
            checkTIDList.DoAdd(cardData.TID);
            ++applyCardCnt;            
        }

        // 필요한 수만큼 카드정보 확보되면 그만
        if (dataDispatchMission.needCardCnt <= applyCardCnt)
            break;

    } while (false == m_Idx.ChangeAndCheckEndIdx());
    
    // 카드갯수가 모자라면
    if (dataDispatchMission.needCardCnt > applyCardCnt)
        return false;

    // UR 카드가 필요갯수 충족 못하면 
    if (dataDispatchMission.NeedURCnt > applyCardURCnt)
        return false;

    // 파견에 진행할 서포터의 최대개수를 넘으면 
    if (eCardSlotPosMax::DISPATCH < applyCardCnt)
        return false;
    

    for (std::vector<uint64_t>::iterator idx = cardUIDs.begin(); idx < cardUIDs.end(); ++idx)
    {
        _outPkt.cardUIDs_.DoAdd(*idx);
    }

    _outPkt.dispatchTID_ = dispatchData.tableID_;

    return true;
}

bool AutoPacketMsg::ChkAndGetDispatchOperConfirmPkt(PktInfoDispatchOperConfirmReq& _outPkt) const
{
    // 인벤토리 체크
    if (false == CanAddItemSlot())
        return false;

    // 우선은 현재 파견중인 데이터중에 순차로 검사하여 완료된 파견의 TID 만..
    const DISPATCH_VECMAP& userDispatchList{ m_DummyUserInfo->GetDispatchList() };
    c_size_t dispatchSize{ userDispatchList.size() };

    if (1 > dispatchSize)
        return false;

    for (uint32_t idx{}; idx < dispatchSize; ++idx)
    {
        const DispatchData& dispatchData{ userDispatchList.GetDataWithIdx(idx) };

        // OperTime 이 0이거나 완료시간이 안되었으면 Continue
        if (false == dispatchData.operEndTime.IsAbove(ProjectMgr::ms_Inst->GetNowDateTimeNum())
            || dispatchData.operEndTime.IsZero())
            continue;

        // 완료된것이 발견되는데로
        _outPkt.dispatchTID_ = dispatchData.TID;

        break;
    }

    if (0 == _outPkt.dispatchTID_)
        return false;

    return true;
}

bool AutoPacketMsg::ChkAndGetStorePurchaseTID(PktInfoStorePurchaseReq& _outPkt) const
{
    if (false == CheckCanAddUserItem())  return false;

    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    _outPkt.storeID_    = userData.GetPossFreeStoreTID(ProjectMgr::ms_Inst->GetNowDateTimeNum());
    if (0 != _outPkt.storeID_)  return true;

    const GOODS_ARR& goodsData{ m_DummyUserInfo->GetGoodsData() };
    auto _work          = [this, &goodsData, &_outPkt](const DataDefault& _data)
    {
                        const DataStore* storeData  = static_cast<const DataStore*>(&_data);

                        // 세일 및 게릴라 상점 관련 처리 위키 링크 참조 -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/할인_상품_관리
                        switch (storeData->SaleType)
                        {
                        case 1:
                        case 2: {} return;
                        }

                        const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
						switch (static_cast<eREWARDTYPE>(storeData->ProductType))
						{
						case eREWARDTYPE::CHAR: {

							if (true == charList.CanGetDataWithKey(storeData->ProductIndex))
								return;
						} break;
						case eREWARDTYPE::COSTUME: {
							const COSTUME_VEC& costumeList{ m_DummyUserInfo->GetCostumeList() };
							for (const auto& cosTID : costumeList)
							{
								if (cosTID == storeData->ProductIndex)
									return;
							}

							uint32_t cosCharID{ TableCostume::Inst->GetData(storeData->PurchaseIndex).CharacterID };
							c_size_t charSize{ charList.size() };
							if (false == charList.CanGetDataWithKey(cosCharID))
								return;
						} break;
						case eREWARDTYPE::USERMARK: {
							const UserData& user{ m_DummyUserInfo->GetUserData() };
							for (const auto& markID : user.markInfos)
							{
								if (markID == storeData->ProductIndex)
									return;
							}
						} break;
						default: { return; }  break;
						}

                        // 염원의 기운 재화 확인.
                        if (true == storeData->IsCanNeedDesirePT())
                        {
                            if (goodsData[eGOODSTYPE::DESIREPOINT] < storeData->NeedDesirePoint)
                                return ;
                        }

                        const eREWARDTYPE costType  = static_cast<eREWARDTYPE>(storeData->PurchaseType);
                        // 상점 비용 타입에 따라  비용 계산 및 소모 확인
                        switch (costType)
                        {
                        case eREWARDTYPE::GOODS: {
                            const eGOODSTYPE costGoodsTP{ static_cast<eGOODSTYPE>(storeData->PurchaseIndex) };
                            if (eGOODSTYPE::_END_ < costGoodsTP)
								return;

                            if (storeData->PurchaseValue <= goodsData[costGoodsTP])
                                _outPkt.storeID_    = storeData->ID;
                        } break;
                        case eREWARDTYPE::ITEM: {
                            if (true == ChkCanUseItemByTIDAndGetUID(storeData->PurchaseIndex, storeData->ProductValue)) 
                                _outPkt.storeID_    = storeData->ID;
                   
                        } break;
                        default: { return; }  break;
                        }
                    };

    for (int i = 0; i < 5; ++i)
    {
        TableStore::Inst->DoWorkToRandData(_work);
        if (0 != _outPkt.storeID_)
            return true;
    }
    return false;
}

bool AutoPacketMsg::ChkAndGetMailTakeUIDListPkt(PktInfoUIDList & _outPkt) const
{
    if (false == CheckCanAddUserItem())
		return false;

    const MailData mailData{ m_DummyUserInfo->GetMailList() };
    if (true == mailData.mailList.empty())
		return false;

    for (const auto& mailInfo : mailData.mailList)
    {
        if (false == mailInfo.second.endTime.IsCheckOverYMD(ProjectMgr::ms_Inst->GetNowDateTimeNum()))
        {
            _outPkt.DoAdd(mailInfo.second.UID);
			return true;
        }
    }
	//if (false == _outPkt.IsEmpty())
	//	return true;
    return false;
}

bool AutoPacketMsg::ChkAndGetMailCount(uint8_t & _outCnt, uint8_t& ) const
{
    const MailData mailData{ m_DummyUserInfo->GetMailList() };
    if (0 >= mailData.maxCnt)
		return false;
    _outCnt = static_cast<uint8_t>(mailData.maxCnt);
    if (20 < _outCnt)
		_outCnt			= 20;

    return true;
}

bool AutoPacketMsg::ChkAndGetCommunityUserArenaInfo(PktInfoCommuUserArenaInfoReq& _outPkt) const
{
    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };
    PktInfoUIDList friendUIDList{};

    if (0 == friendList.size())
        return false;

    const uint32_t randCnt{ static_cast<uint32_t>(ProjectMgr::ms_Inst->GetRand(0, friendList.size())) };

    for (uint32_t idx{}; idx < randCnt; ++idx)
    {
        FriendData friendData{ friendList.GetDataWithIdx(idx) };

        friendUIDList.DoAdd(friendData.info.uuid);
    }

    _outPkt.reqUuid_ = userData.UUID;
    _outPkt.uids_ = friendUIDList;

    return true;
}
bool AutoPacketMsg::ChkAndGetCommunityUserUseCallCnt(PktInfoCommuUseCallCntReq& _outPkt) const
{
    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };
    PktInfoUIDList friendUIDList{};

    if (0 == friendList.size())
        return false;

    const uint32_t randCnt{ static_cast<uint32_t>(ProjectMgr::ms_Inst->GetRand(0, friendList.size())) };

    for (uint32_t idx{}; idx < randCnt; ++idx)
    {
        FriendData friendData{ friendList.GetDataWithIdx(idx) };

        friendUIDList.DoAdd(friendData.info.uuid);
    }

    _outPkt.reqUuid_ = userData.UUID;
    _outPkt.uids_ = friendUIDList;

    return false;
}
bool AutoPacketMsg::ChkAndGetFriendSuggestlist(PktInfoCommuSuggestReq& _outPkt) const
{
    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    //const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };
    PktInfoUIDList friendUIDList{};

    //uint32_t randCnt = static_cast<uint32_t>(ProjectMgr::ms_Inst->GetRand(0, 5));
    
    _outPkt.reqUuid_ = userData.UUID;
    _outPkt.sugUids_ = friendUIDList;

    return true;
}
bool AutoPacketMsg::ChkAndGetFriendAskPkt(PktInfoCommuAskReq& _outPkt) const
{
	const COMMUNITY_VECMAP& suggestList{ m_DummyUserInfo->GetFriSuggestList() };

	size_t sugSize{ suggestList.size() };
	if (0 == sugSize)
		return false;
	
	const FRIEND_VECMAP& answerList{ m_DummyUserInfo->GetFriFromList() };
	const FRIEND_VECMAP& askList{ m_DummyUserInfo->GetFriAskList() };
	const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };

	if (TableBalance::Inst->Get().FriendAddMaxNumber <= friendList.size())
		return false;
	if (TableBalance::Inst->Get().FriendAskMaxNumber <= askList.size())
		return false;

	for (size_t sugIdx{}; sugIdx < sugSize; ++sugIdx)
	{
		Uuid_t uuid{ suggestList.GetDataWithIdx(sugIdx).uuid };
		if (uuid == 0)
			continue;
		if (false == ChkFriendListCanAddUuid(answerList, uuid))
			continue;
		if (false == ChkFriendListCanAddUuid(askList, uuid))
			continue;
		if (false == ChkFriendListCanAddUuid(friendList, uuid))
			continue;

		_outPkt.tgtUids_.DoAdd(uuid);

		//STMaxLenFmtString log{};
		//log.Format(_TX("ChkAndGetFriendAskPkt - user:%llu | friend:%llu"), m_DummyUserInfo->GetUserData().UUID, _outPkt.tgtUids_.uids_[0]);
		//ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_CHK_);
		return true;
	}
	
    return false;
}
bool AutoPacketMsg::ChkAndGetFriendAskDelPkt(PktInfoCommuAskDel& _outPkt) const
{
	const FRIEND_VECMAP& askList{ m_DummyUserInfo->GetFriAskList() };
	size_t size{ askList.size() };

	if (0 == size)
		return false;

	FriendData data{ askList.GetDataWithIdx(ProjectMgr::ms_Inst->GetRand(0, size - 1)) };
	_outPkt.tgtUids_.DoAdd(data.info.uuid);

	//STMaxLenFmtString log{};
	//log.Format(_TX("ChkAndGetFriendAskDelPkt - user:%llu | friend:%llu"), m_DummyUserInfo->GetUserData().UUID, _outPkt.tgtUids_.uids_[0]);
	//ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_CHK_);

    return true;
}
bool AutoPacketMsg::ChkAndGetFriendAnswer(PktInfoCommuAnswer& _outPkt) const
{
	const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };
	if (TableBalance::Inst->Get().FriendAddMaxNumber <= friendList.size())
		return false;

	const FRIEND_VECMAP& answerList{ m_DummyUserInfo->GetFriFromList() };
	if (0 == answerList.size())
		return false;

	_outPkt.tgtUids_.DoAdd(answerList.GetDataWithIdx(0).info.uuid);
	_outPkt.accept_		= (0 == ProjectMgr::ms_Inst->GetRand(0, 1)) ? true : false;

	//STMaxLenFmtString log{};
	//log.Format(_TX("ChkAndGetFriendAnswer - user:%llu | friend:%llu"), m_DummyUserInfo->GetUserData().UUID, _outPkt.tgtUids_.uids_[0]);
	//ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_INF_);
	//ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_CHK_);

	return true;
}
bool AutoPacketMsg::ChkAndGetFriendKick(PktInfoCommuKick& _outPkt) const
{
	const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };

	if (0 == friendList.size())
		return false;

	_outPkt.tgtUids_.DoAdd(friendList.GetDataWithIdx(0).info.uuid);

	//STMaxLenFmtString log{};
	//log.Format(_TX("ChkAndGetFriendKick - user:%llu | friend:%llu"), m_DummyUserInfo->GetUserData().UUID, _outPkt.tgtUids_.uids_[0]);
	//ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_INF_);

	return true;
}
bool AutoPacketMsg::ChkAndGetFriendGive() const
{
	const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };

	if (0 == friendList.size())
		return false;

	//STMaxLenFmtString log{};
	//log.Format(_TX("ChkAndGetFriendPointTake - user:%llu | friendSize:%llu"), m_DummyUserInfo->GetUserData().UUID, friendList.size());
	//ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_INF_);

	const GOODS_ARR& goodsList{ m_DummyUserInfo->GetGoodsData() };
	if (TableBalance::Inst->Get().LimitMaxFP <= goodsList[eGOODSTYPE::FRIENDPOINT])
		return false;

    return m_DummyUserInfo->GetUserData().CanSendFriPoint(ProjectMgr::ms_Inst->GetNowDateTimeNum());
}
bool AutoPacketMsg::ChkAndGetFriendPointTake(PktInfoFriendPointTakeReq& _outPkt) const
{
	const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };

	size_t size{ friendList.size() };

	if (0 == friendList.size())
		return false;

		const GOODS_ARR& goodsList{ m_DummyUserInfo->GetGoodsData() };
	if (TableBalance::Inst->Get().LimitMaxFP <= goodsList[eGOODSTYPE::FRIENDPOINT])
		return false;

	for (size_t idx{}; idx < size; ++idx)
	{
		const FriendData& data{ friendList.GetDataWithIdx(idx) };
		if (true == data.CanGetFriPoint())
		{
			_outPkt.tgtUids_.DoAdd(data.info.uuid);

			//STMaxLenFmtString log{};
			//log.Format(_TX("ChkAndGetFriendPointTake - user:%llu | friend:%llu"), m_DummyUserInfo->GetUserData().UUID, _outPkt.tgtUids_.uids_[0]);
			//ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_INF_);

			return true;
		}
	}

    return false;
}
bool AutoPacketMsg::ChkAndGetFriendRoomVisitFlag(PktInfoFriendRoomFlag& _outPkt) const
{
	const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };

	size_t size{ friendList.size() };
	if (0 == size)
		return false;

	FriendData data{ friendList.GetDataWithIdx(ProjectMgr::ms_Inst->GetRand(0, size - 1)) };
	_outPkt.tgtUids_.DoAdd(data.info.uuid);
	_outPkt.accept_ = (true == data.CanVisitMyRoom()) ? false : true;

	//STMaxLenFmtString log{};
	//log.Format(_TX("ChkAndGetFriendRoomVisitFlag - user:%llu | friend:%llu"), m_DummyUserInfo->GetUserData().UUID, _outPkt.tgtUids_.uids_[0]);
	//ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_INF_);
	return true;
}
bool AutoPacketMsg::ChkAndGetFriendRoomInfoGet(PktInfoCommuRoomInfoGet& _outPkt) const
{
	const FRIEND_VECMAP& friendList{ m_DummyUserInfo->GetFriList() };

	size_t size{ friendList.size() };
	if (0 == size)
		return false;

	FriendData data{ friendList.GetDataWithIdx(ProjectMgr::ms_Inst->GetRand(0, size - 1)) };
	if (true == data.CanVisitFriRoom())
	{
		_outPkt.tgtUuid_ = data.info.uuid;
		//STMaxLenFmtString log{};
		//log.Format(_TX("ChkAndGetFriendRoomInfoGet - user:%llu | friend:%llu"), m_DummyUserInfo->GetUserData().UUID, _outPkt.tgtUuid_);
		//ProjectMgr::ms_Inst->DoLog(log.Get(), LogType::_INF_);
		return true;
	}

    return false;
}

bool AutoPacketMsg::ChkFriendListCanAddUuid(const FRIEND_VECMAP& _list, c_Uuid _uuid) const
{
	size_t size{ _list.size() };
	if (0 == size)
		return true;

	for (size_t idx{}; idx < size; ++idx)
	{
		FriendData data{ _list.GetDataWithIdx(idx) };
		if (_uuid == data.info.uuid)
			return false;
	}

	return true;
}

bool AutoPacketMsg::ChkAndGetChangeMainRoomSlot(uint8_t & _outSlot) const
{
    const ROOM_VEC& roomList{ m_DummyUserInfo->GetRoomList() };
    const UserData& userData{ m_DummyUserInfo->GetUserData() };
    if (1 >= roomList.size())
		return false;
    
    while (_outSlot == userData.roomSlot)
        _outSlot        = static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, roomList.size() - 1));
    return true;
}

bool AutoPacketMsg::ChkAndGetRoomStoreTID(uint32_t & _outTID) const
{
    auto _work          = [this, &_outTID](c_uint64, const DataDefault& _data) 
                    {
                        const DataStoreRoom& storeRoomData{ static_cast<const DataStoreRoom&> (_data) };

                        const GOODS_ARR& goodsData{ m_DummyUserInfo->GetGoodsData() };
                        if (goodsData[storeRoomData.PurchaseType] < storeRoomData.PurchaseValue
                            || goodsData[eGOODSTYPE::ROOMPOINT] < storeRoomData.NeedRoomPoint)
                            return false;

                        const ROOM_PURCHASE_MAP& roomPurchaseList{ m_DummyUserInfo->GetRoomPurchaseList() };
                        const eREWARDTYPE productType{ static_cast<eREWARDTYPE>(storeRoomData.ProductType) };
                        ROOM_PURCHASE_MAP::const_iterator typeListitor{ roomPurchaseList.find(productType) };
                        if (roomPurchaseList.end() != typeListitor)
                        {
                            const RoomPurchaseData& typeList{ typeListitor->second };
                            for (const auto& productIdx : typeList)
                            {
                                if (productIdx == storeRoomData.ProductIndex)
									return false;
                            }
                        }

                        bool canBuy             = false;
                        switch (productType)
                        {
                        case eREWARDTYPE::ROOMFIGURE:
                        {
                            const DataRoomFigure& roomFigureData{ TableRoomFigure::Inst->GetData(storeRoomData.ProductIndex) };
                            if (static_cast<eContentsPosKind>(roomFigureData.ContentsType) == eContentsPosKind::COSTUME)
                            {
                                const COSTUME_VEC& costumeList{ m_DummyUserInfo->GetCostumeList() };
                                for (const auto& costumeData : costumeList)
                                {
                                    if (costumeData == roomFigureData.ContentsIndex)
                                    {
                                        canBuy  = true;
                                        break;
                                    }
                                }
                            }
                            else if (static_cast<eContentsPosKind>(roomFigureData.ContentsType) == eContentsPosKind::MONSTER)
                            {
                                const BOOK_VECMAP& monsterBookList{ m_DummyUserInfo->GetMonsterBookList() };
                                if (monsterBookList.CanGetDataWithKey(roomFigureData.ContentsIndex))
                                    canBuy      = true;
                            }
                        }   break;
                        case eREWARDTYPE::ROOMACTION:
                        {
                            const CHAR_VECMAP& charList{ m_DummyUserInfo->GetCharList() };
                            const DataRoomAction& roomActionData{ TableRoomAction::Inst->GetData(storeRoomData.ProductIndex) };
							if (true == charList.CanGetDataWithKey(roomActionData.CharacterID))
							{
								canBuy			= true;
								break;
							}
                        }   break;
                        case eREWARDTYPE::ROOMFUNC:
                        {
                            const DataRoomFunc& roomFuncData{ TableRoomFunc::Inst->GetData(storeRoomData.ProductIndex) };
                            typeListitor = roomPurchaseList.find(eREWARDTYPE::ROOMTHEME);
                            if (roomPurchaseList.end() != typeListitor)
                            {
                                const RoomPurchaseData& typeList{ typeListitor->second };
                                for (const auto& productIdx : typeList)
                                {
                                    if (productIdx == roomFuncData.RoomTheme)
                                    {
                                        canBuy  = true;
                                        break;
                                    }
                                }
                            }
                        }   break;
                        default: { canBuy       = true; } break;
                        }

                        if (canBuy == false)
							return false;

                       
                        _outTID             = storeRoomData.ID;
                        return true;
                    };
    return TableStoreRoom::Inst->DoWorkToDatas(_work);
}

bool AutoPacketMsg::ChkAndGetReqDetailInfoSlot(uint8_t & _outSlot) const
{
    const ROOM_VEC& roomList{ m_DummyUserInfo->GetRoomList() };
    if (0 >= roomList.size())   return false;
    _outSlot            = static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, roomList.size() - 1));
    return true;
}

bool AutoPacketMsg::ChkAndGetRoomSlotSavePkt(PktInfoRoomThemeSlotDetail & _outPkt) const
{
    const ROOM_VEC& roomList{ m_DummyUserInfo->GetRoomList() };
    const ROOM_PURCHASE_MAP& roomPurchaseList{ m_DummyUserInfo->GetRoomPurchaseList() };
    c_uint8 maxRoomSlot{ TableBalance::Inst->Get().MaxServerRoomSaveSlot };

    if (maxRoomSlot <= 0)
        return false;

    if (1 == maxRoomSlot)
        _outPkt.themeSlot_.slotNum_ = 0;
    else
        _outPkt.themeSlot_.slotNum_                 = static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, maxRoomSlot - 1));

    const RoomPurchaseData typeRoomThemeList{ roomPurchaseList.find(eREWARDTYPE::ROOMTHEME)->second };
    if (0 == typeRoomThemeList.size())
		return false;

    _outPkt.themeSlot_.tableID_ = typeRoomThemeList[ProjectMgr::ms_Inst->GetRand(0, typeRoomThemeList.size() - 1)];
    if (0 == _outPkt.themeSlot_.tableID_)
		return false;
    _outPkt.themeSlot_.data_.DoAdd("data", sizeof("data"));

    if (0 < roomPurchaseList.count(eREWARDTYPE::ROOMFUNC))
    {
        const RoomPurchaseData & typeRoomFuncList{ roomPurchaseList.find(eREWARDTYPE::ROOMFUNC)->second };
        for (const auto& roomFunc : typeRoomFuncList)
        {
            if (roomList[_outPkt.themeSlot_.slotNum_].funcState != roomFunc &&
                TableRoomFunc::Inst->GetData(roomFunc).RoomTheme == _outPkt.themeSlot_.tableID_)
            {
                _outPkt.themeSlot_.funcStateFlag_   = roomFunc;
                break;
            }
        }
    }

    if (0 < roomPurchaseList.count(eREWARDTYPE::ROOMFIGURE))
    {
        RoomPurchaseData typeFigureList{ roomPurchaseList.find(eREWARDTYPE::ROOMFIGURE)->second };
        uint8_t maxFigure{ TableRoomTheme::Inst->GetData(_outPkt.themeSlot_.tableID_).MaxChar };
        for (uint8_t idx = 0; idx < maxFigure; ++idx)
        {
            if (0 >= typeFigureList.size())
                break;

            PktInfoRoomFigureSlot::Piece figurePkt{};
            figurePkt.figureSlotNum_    = idx;
            uint8_t randFigure{ static_cast<uint8_t>(ProjectMgr::ms_Inst->GetRand(0, typeFigureList.size() - 1)) };
            figurePkt.tableID_          = typeFigureList[randFigure];
            typeFigureList.erase(typeFigureList.begin() + randFigure);
            figurePkt.detail_.DoAdd("detail", sizeof("detail"));

            if (static_cast<eContentsPosKind>(TableRoomFigure::Inst->GetData(figurePkt.tableID_).ContentsType) == eContentsPosKind::MONSTER)
            {
                _outPkt.figureSlot_.infos_.push_back(figurePkt);
                continue;
            }

            auto _work  = [this, &figurePkt](c_uint64, const DataDefault& _data) {

                        if (figurePkt.actionID1_ != 0 && figurePkt.actionID2_ != 0)
                            return true;

                        const DataRoomAction& roomActionData{ static_cast<const DataRoomAction&> (_data) };
                        if (TableRoomFigure::Inst->GetData(figurePkt.tableID_).CharacterID == roomActionData.CharacterID)
                        {
                            if (roomActionData.Type == eRoomActionType::FACE && figurePkt.actionID1_ == 0)
                                figurePkt.actionID1_    = roomActionData.ID;
                            else if (roomActionData.Type == eRoomActionType::BODY && figurePkt.actionID2_ == 0)
                                figurePkt.actionID2_    = roomActionData.ID;
                        }

                        return false;
                    };
            if (true == TableRoomAction::Inst->DoWorkToDatas(_work))
				_outPkt.figureSlot_.infos_.push_back(figurePkt);
        }
    }
    m_DummyUserInfo->m_nowSetRoomSlot       = _outPkt;
    return true;
}

bool AutoPacketMsg::TestStoreIDChk(uint32_t _storeID) const
{
	if (false == CheckCanAddUserItem())  
		return false;

    const DataStore& storeData{ TableStore::Inst->GetData(_storeID) };
    const GOODS_ARR& goodsData{ m_DummyUserInfo->GetGoodsData() };

    switch (static_cast<eREWARDTYPE>(storeData.PurchaseType))
    {
    case eREWARDTYPE::GOODS: {
        const eGOODSTYPE costGoodsTP{ static_cast<eGOODSTYPE>(storeData.PurchaseIndex) };
        if (eGOODSTYPE::_END_ < costGoodsTP)
			return true;

        if (goodsData[costGoodsTP] >= storeData.PurchaseValue) 
            return true;
    } break;
    case eREWARDTYPE::ITEM: {
        if (true == ChkCanUseItemByTIDAndGetUID(storeData.PurchaseIndex, storeData.ProductValue))  
            return true;

    } break;
    default: { return false; }  break;
    }
    return false;
}
