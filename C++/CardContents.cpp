#include "stdafx.h"

#include "CardContents.h"

#include "ProductMgr.h"
#include "contents/facility/FacilityContents.h"
#include "contents/book/BookMgr.h"

#include "actor/user/UserCommon.h"
#include "actor/character/CharacterPack.h"

#include "table/TableCard.h"
#include "table/TableBalance.h"
#include "table/TableItemReqList.h"
#include "table/TableLevelUp.h"
#include "table/TableRandom.h"


CardContents::InstVec  CardContents::ms_InstVec;
CardContents::CardContents()
: ContentsBase()
{
    m_End               = m_Cont.end();
    m_BookEnd           = m_BookCont.end();
}
CardContents::~CardContents()
{
}

void CardContents::_DoProcess_(c_uint64 /*_deltaTmMs*/, const eUpdateTickKind::ENUM /*_tickKind*/)
{
}
void CardContents::_DoRelease_()
{
    if (nullptr == m_Mgr)
        return ;
    for (const auto& info : m_Cont)
        m_Mgr->DoPushToCardPool(info.second);
    m_Cont.clear();
    m_End               = m_Cont.end();

    for (const auto& info : m_BookCont)
        m_BookMgr->DoPushToCardBookPool(info.second);
    m_BookCont.clear();
    m_BookEnd           = m_BookCont.end();

    for (auto& posVec : m_PosContVec)
    {
        for (auto& itr : posVec)
        {
            InstVec* uidVec{ itr.second };
            _safe_delete(uidVec);
        }
        posVec.clear();
    }
    m_PosContVec.clear();

    m_Mgr               = nullptr;
}

void CardContents::DoInit(ProductMgr* _mgr, BookMgr* _bookMgr, const UserCommon* _user, const PktInfoDetailUser& _pkt)
{
    m_Mgr               = _mgr;
    m_BookMgr           = _bookMgr;
    _DoSetInit(_user);

    m_PosContVec.clear();
    m_PosContVec.resize(static_cast<c_size_t>(eContentsPosKind::_MAX_));

    this->DoAdd(_pkt.cards_, _pkt.cardBooks_, true);
}
eTMIErrNum CardContents::DoAdd(const PktInfoCard& _pktInfo, const PktInfoCardBook& _pktBookInfo, const bool _initFlag)
{
    // 카드(서포터) 도감 담기
    const PktInfoCardBook::Pack& bookPack{ _pktBookInfo.infos_ };
    size_t size{ bookPack.size() };
    for (size_t loop{}; loop < size; ++loop)
    {
        const PktInfoCardBook::Piece& piece{ bookPack[loop] };
        PktInfoCardBook::Piece* popData{ m_BookMgr->DoPopFromCardBookPool() };
        (*popData)      = piece;
        m_BookCont.insert(BookCont::value_type(popData->tableID_, popData));
    }
    m_BookEnd           = m_BookCont.end();

    // 카드(서포터) 담기
    const PktInfoCard::Pack& pack{ _pktInfo.infos_ };
    size                = pack.size();
    for (size_t loop{}; loop < size; ++loop)
    {
        const PktInfoCard::Piece& piece{ pack[loop] };
        c_uint32 tableID{ piece.tableID_ };
        PktInfoCard::Piece* popData{ m_Mgr->DoPopFromCardPool() };
        (*popData)      = piece;
        popData->book_  = __GetBookInst(tableID, _MAC_FUNC_);
        m_Cont.insert(Cont::value_type(popData->cardUID_, popData));

        // 사용중인 카드(서포터) 정보를 따로 담아 둡니다.
        if (eContentsPosKind::_NONE_ != piece.posKind_)
            __DoAddPosValue(popData, popData->posKind_, popData->posValue_, popData->posSlotNum_);

        // 서포터 정보 초기화 시점이 아닐 경우에만 공적 관련 처리를 합니다.
        if (false == _initFlag)
        {
            // 유저 공적(특정 등급 카드(서포터) 획득 횟수)
            AutoAchieveCondiWorker achieveWorker(*_GetUser(), eAchieveType::AM_Col_GetSpt, popData->Table().Grade, 1, _MAC_FUNC_);

            // 게릴라 미션(특정 카드(서포터) 획득)
            AutoGllaMissionWorker gllaWorker(*_GetUser(), eGuerrillaMissionType::GM_SptTIDGet_Cnt, tableID, 1, _MAC_FUNC_);
        }
    }
    m_End               = m_Cont.end();
    return eTMIErrNum::SUCCESS_OK;
}
void CardContents::GetInfo(PktInfoCard* _pkt, PktInfoCardBook* _pktBook) const
{
    _pkt->DoRelease();
    PktInfoCard::Pack& pack{ _pkt->infos_ };
    pack.reserve(m_Cont.size());
    for (const auto& info : m_Cont)
        pack.push_back(*info.second);

    _pktBook->DoRelease();
    PktInfoCardBook::Pack& bookPack{ _pktBook->infos_ };
    bookPack.reserve(m_BookCont.size());
    for (const auto& info : m_BookCont)
        bookPack.push_back(*info.second);
}
void CardContents::DoTableReloadAll()
{
    for (auto& info : m_Cont)
        info.second->DoResetTable();
    for (auto& info : m_BookCont)
        info.second->DoResetTable();
}
void CardContents::DoSvrInfoReload()
{
}

void CardContents::GetDetailInfo(PktInfoConPosCharDetail* _outInfo, c_Cuid _cuid) const
{
    if (INVALID_UID == _cuid)
        return;

    const InstVec& instVec{ __GetInstVecToPosValue(eContentsPosKind::CHAR, _cuid) };
    size_t size{ instVec.size() };
    if (eCardSlotPosMax::_SLOT_MAX_ < size)
        size            = eCardSlotPosMax::_SLOT_MAX_;
    for (size_t loop{}; loop < size; ++loop)
    {
        const PktInfoCard::Piece* inst{ instVec[loop] };
        if (true == inst->IsNull())
            continue;

        PktInfoConPosCharDetail::ComInfo& info{ _outInfo->cards_[loop] };
        info.ID_        = inst->tableID_;
        info.lv_        = inst->lv_;
        info.sklLv_     = inst->skillLv_;
        info.wake_      = inst->wake_;
    };
}

eTMIErrNum CardContents::IsCanApplyPosCard(PktInfoCardApplyPos* _outPkt) const
{
    const PktInfoCard::Piece& piece{ __GetInst(_outPkt->cardUID_) };
    if (true == piece.IsNull())                 return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;

    // 기존과 같은 위치인지 확인합니다.
    c_uint8 dstPosSlotNum{ _outPkt->slotNum_ };
    c_uint64 dstPosVal{ _outPkt->posValue_ };
    const eContentsPosKind dstPosKind{ _outPkt->posKind_ };
    const eContentsPosKind applyCardOldPosKind{ piece.posKind_ };
    c_uint64 applyCardOldPosValue{ piece.posValue_ };
    c_uint8 applyCardOldPosSlotNum{ piece.posSlotNum_ };
    if (dstPosKind == applyCardOldPosKind)
    {
        if (dstPosVal == applyCardOldPosValue && dstPosSlotNum == applyCardOldPosSlotNum)
            return eTMIErrNum::SVR_CARD_POS_ERR_ALREADY_SAME_USED;
    }

    // 적용 하려는 카드의 교체 허용 가능 체크(캐릭터가 아닌 다른 콘텐츠에서 사용중이라면 교체 허용 하지 않습니다.)
    if (eContentsPosKind::_NONE_ != applyCardOldPosKind && eContentsPosKind::CHAR != applyCardOldPosKind)
        return eTMIErrNum::SVR_CARD_POS_ERR_IMPOSSIBLE_CHANGE_POS;

    uint64_t maxSlotSize{};
    switch (dstPosKind)
    {
    case eContentsPosKind::CHAR :
        {
            maxSlotSize = eCardSlotPosMax::CHAR;

            if (false == _GetUser()->GetChars().IsExistInst(dstPosVal, false) )
                return eTMIErrNum::SVR_CHAR_COM_ERR_NOT_EXIST_INST;
        } break;
    case eContentsPosKind::FACILITY :
        {
            maxSlotSize = eCardSlotPosMax::FACILITY;

            const eTMIErrNum err{ _GetUser()->GetFacis().IsCanApplyUseCard(dstPosVal) };
            if (eTMIErrNum::SUCCESS_OK != err)
                return err;
        } break;
    default :           { return eTMIErrNum::SVR_CARD_POS_ERR_APPLY_UNKNOWN_POS_KIND; } break;
    }
    if (maxSlotSize <= dstPosSlotNum)           return eTMIErrNum::SVR_CARD_POS_ERR_OVER_MAX_POS_SLOT_NUM;

    const InstVec& instVec{ __GetInstVecToPosValue(dstPosKind, dstPosVal) };
    // 아직 추가된 적이 없는 위치 적용 슬롯이라면 기존 위치의 교체나 탈착 처리를 할 필요가 없어서 바로 성공으로 처리합니다.
    if (true == instVec.empty())
        return eTMIErrNum::SUCCESS_OK;

    c_uint64 applyCardUID{ piece.cardUID_ };
    c_uint32 applyCardTID{ piece.tableID_ };
    const PktInfoCard::Piece* _oldInstInSlot{ &PktInfoCard::Piece::ms_Null };
    for (uint64_t loop{}; loop < maxSlotSize; ++loop)
    {
        // 같은 위치에 적용 중인 카드(서포터)의 슬롯 변경이라면 기존 슬롯은 확인하지 않습니다.
        if (applyCardOldPosValue == dstPosVal && applyCardOldPosSlotNum == loop)
            continue;

        const PktInfoCard::Piece& instInSlot{ *instVec[loop] };
        if (loop == dstPosSlotNum)
        {
            _oldInstInSlot  = &instInSlot;
            continue;
        }

        if (true == instInSlot.IsNull())
            continue;

        // 같은 종류의 카드(서포터)가 이미 적용중이라면 처리하지 않습니다.
        if (applyCardTID == instInSlot.tableID_)
            return eTMIErrNum::SVR_CARD_POS_ERR_ALREADY_SAME_USED_IN_SLOT;
    }

    // 기존 슬롯 위치에 다른 카드(서포터)가 있다면 탈착 처리를 합니다.
    if (false == _oldInstInSlot->IsNull())
    {
        _outPkt->oldCardUID_            = _oldInstInSlot->cardUID_;
        _outPkt->oldCardChangeSlotNum_  = eCardSlotPosMax::_SLOT_MAX_;
        // 같은 위치 적용 중에 슬롯이동이라면 교체 처리를 합니다.
        if (eContentsPosKind::_NONE_ != applyCardOldPosKind && applyCardOldPosValue == dstPosVal)
            _outPkt->oldCardChangeSlotNum_  = applyCardOldPosSlotNum;
    }
    return eTMIErrNum::SUCCESS_OK;
}

eTMIErrNum CardContents::IsCanApplyPosCards(PktInfoContentsSlotPos* _outPkt, const PktInfoUIDList& _pktInfo, const eContentsPosKind _posKind, c_uint64 _posVal, LAMBDA(eTMIErrNum(const PktInfoCard::Piece& _piece, c_size_t _slotNum), _checkWork)) const
{
    if (true == _pktInfo.IsCheckDuplicateInList())          return eTMIErrNum::COM_ERR_DUPLICATE_IN_LIST;

    eTMIErrNum err{ eTMIErrNum::SUCCESS_OK };
    PktInfoTIDList sameCheckTIDList;
    PktInfoTIDList::Pack* sameCheckPack{ &sameCheckTIDList.tids_ };
    PktInfoContentsSlotPos::Piece addSlotPos;
    c_size_t size{ _pktInfo.uids_.size() };
    for (size_t loop{}; loop < size; ++loop)
    {
        c_uint64 uid{ _pktInfo.uids_[loop] };
        const PktInfoCard::Piece& piece{ __GetInst(uid) };
        if (true == piece.IsNull())                         return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;

        // 이미 위치 적용중인 카드(서포터)인지 확인합니다.
        if (eContentsPosKind::_NONE_ != piece.posKind_)     return eTMIErrNum::SVR_CARD_POS_ERR_ALREADY_APPLY_USED;

        // 적용 확인중인 목록 내에 같은 종류의 카드(서포터)가 이미 적용중이라면 처리하지 않습니다.
        c_uint32 tid{ piece.tableID_ };
        if (true == sameCheckTIDList.IsExistSameTID(tid))   return eTMIErrNum::SVR_CARD_POS_ERR_ALREADY_SAME_USED_IN_SLOT;

        sameCheckPack->push_back(tid);

        err                 = _checkWork(piece, loop);
        if (eTMIErrNum::SUCCESS_OK != err)                  return err;

        addSlotPos.uid_     = uid;
        addSlotPos.kind_    = _posKind;
        addSlotPos.value_   = _posVal;
        addSlotPos.slotNum_ = static_cast<c_uint8>(loop);
        _outPkt->infos_.push_back(addSlotPos);
    }
    return eTMIErrNum::SUCCESS_OK;
}
eTMIErrNum CardContents::IsCanApplyOutPosCard(PktInfoCardApplyOutPos* _outPkt, c_uint64 _uid) const
{
    const PktInfoCard::Piece& piece{ __GetInst(_uid) };
    if (true == piece.IsNull())                 return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;
    const eContentsPosKind outPosKind{ piece.posKind_ };
    if (eContentsPosKind::_NONE_ == outPosKind) return eTMIErrNum::SVR_CARD_POS_ERR_ALREADY_OUT_NOT_RE_OUT;

    c_uint64 outPosValue{ piece.posValue_ };
    switch (outPosKind)
    {
    case eContentsPosKind::FACILITY :
        {
            const eTMIErrNum err{ _GetUser()->GetFacis().IsCanApplyOutPosCard(outPosValue) };
            if (eTMIErrNum::SUCCESS_OK != err)
                return err;
        } break;
    }
    _outPkt->cardUID_   = _uid;
    return eTMIErrNum::SUCCESS_OK;
}
eTMIErrNum CardContents::IsCanSell(PktInfoCardSell::Piece* _outPktInfoPiece, c_uint64 _nowGold, c_uint64 _nowSP) const
{
    const PktInfoCard::Piece* piece{};
    const eTMIErrNum err{ __IsCheckCanSell(_outPktInfoPiece->cardUID_, &piece) };
    if (eTMIErrNum::SUCCESS_OK != err)  return err;

    const DataCard& data{ piece->Table() };
    float addGold{ static_cast<c_float>(data.SellPrice) };
    // 게릴라 캠페인 효과 적용(아이템 판매 시 골드 획득량 증가) -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/게릴라_캠페인
    addGold             += addGold * _GetGllaCampaignValRatio(eGuerrillaCampType::GC_ItemSell_PriceRateUp);
    _outPktInfoPiece->addGold_  = static_cast<c_uint32>(TableBalance::Inst->DoCanAddGoldAndGetButDoNotOverMax(_nowGold, static_cast<c_uint64>(addGold)));
    _outPktInfoPiece->addSP_    = static_cast<c_uint32>(TableBalance::Inst->DoCanAddSPAndGetButDoNotOverMax(_nowSP, data.SellMPoint));
    return err;
}
eTMIErrNum CardContents::IsCanLockFlag(c_uint64 _uid, const bool _lock) const
{
    const PktInfoCard::Piece& piece{ __GetInst(_uid) };
    if (true == piece.IsNull())         return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;

    if (_lock == piece.lock_)
    {
        if (true == _lock)
            return eTMIErrNum::COM_ERR_ALREADY_LOCK_STATE;
        return eTMIErrNum::COM_ERR_ALREADY_UNLOCK_STATE;
    }
    return eTMIErrNum::SUCCESS_OK;
}
eTMIErrNum CardContents::IsCanLvUp(PktInfoCardGrow* _outPkt, PktInfoProductComGrowReq&& _pktInfo, const bool _ignoreCost) const
{
    const PktInfoCard::Piece& piece{ __GetInst(_pktInfo.targetUID_) };

    const TableBalance& tblBal{ *TableBalance::Inst };
    const DataBalance& dataBal{ tblBal.Get() };

    // 게릴라 캠페인 효과 적용(강화 시 골드 소모량 감소) -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/게릴라_캠페인
    c_float costDownRatio{ _GetGllaCampaignValRatio(eGuerrillaCampType::GC_Upgrade_PriceRateDown, static_cast<c_uint32>(eContentsPosKind::CARD)) };
    // 카드(서포터) 강화 부분 참고 -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/서포터#.EA.B0.95.ED.99.94
    // 비용 계산
    c_uint64 myNowGold{ _GetUserGold() };
    uint64_t lvUpCost{};
    if (false == _ignoreCost && costDownRatio < 1.f)
    {
        // 레벨업 비용 소모 확인
        lvUpCost        = (dataBal.CardLevelupCostByLevel * piece.lv_) * _pktInfo.GetTotalStackCnt();
        lvUpCost        -= static_cast<c_uint64>(static_cast<c_float>(lvUpCost) * costDownRatio);
        if (myNowGold < lvUpCost)   return eTMIErrNum::COM_ERR_LACK_GOLD;
    }

    // 재료 확인 및 경험치 계산
    c_float expWeight{ dataBal.CardExpMatWeigth };
    float addExp{};
    auto materWork      = [&addExp, &expWeight](const PktInfoCard::Piece& _mater) -> eTMIErrNum {

                        addExp  += static_cast<c_float>(_mater.Table().Exp) + (static_cast<c_float>(_mater.exp_) * expWeight);
                        return eTMIErrNum::SUCCESS_OK;
                    };
    auto itemWork       = [&addExp](const PktInfoItem::Piece& _piece, c_uint16 _cnt) -> eTMIErrNum {
                        
                        if (false == _piece.IsCanUseMaterial(eITEMSUBTYPE::MATERIAL_CARD_LEVELUP))
                            return eTMIErrNum::COM_ERR_LV_OTHER_TYPE_MATERIAL;

                        addExp  += _piece.GetExpFromValue() * _cnt;
                        return eTMIErrNum::SUCCESS_OK;
                    };
    eTMIErrNum err{ __IsCheckCommonGrow(&_outPkt->comGrow_, _pktInfo, piece, materWork, itemWork, dataBal.MatCount) };
    if (eTMIErrNum::SUCCESS_OK != err)  return err;

    // 게릴라 캠페인 효과 적용(강화 시 경험치 획득량 증가) -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/게릴라_캠페인
    c_float expRatio{ _GetGllaCampaignValRatio(eGuerrillaCampType::GC_Upgrade_ExpRateUP, static_cast<c_uint32>(eContentsPosKind::CARD)) };
    addExp              += addExp * expRatio;

    // 게릴라 캠페인 효과 적용(강화 시 일반 성공 확률 감소) -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/게릴라_캠페인
    c_uint32 normalSuccDownVal{ _GetGllaCampaignVal(eGuerrillaCampType::GC_Upgrade_SucNorRateDown) };
    // 강화 대성공 참고 -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/강화_대성공_시스템
    uint64_t bonusIdx{ static_cast<c_uint64>(eGrowState::SUCC_NORMAL) };
    addExp              *= tblBal.GetRandLvUpBonusRatio(&bonusIdx, normalSuccDownVal);

    // 레벨업 유효성 검사 및 관련 정보 획득
    PktInfoProductComGrowAck* outComGrow{ &_outPkt->comGrow_ };
    err                 = __IsCanChangeExp(&_outPkt->bookState_, &outComGrow->expLv_, piece, static_cast<c_int32>(addExp), true);
    if (eTMIErrNum::SUCCESS_OK != err)  return err;

    outComGrow->DoMoveInit(MOVE_CAST(_pktInfo));
    outComGrow->targetUID_  = _pktInfo.targetUID_;
    outComGrow->userGold_   = myNowGold - lvUpCost;
    _outPkt->retWake_       = piece.wake_;
    _outPkt->retSkillLv_    = piece.skillLv_;
    _outPkt->growState_     = static_cast<eGrowState>(bonusIdx);
    return err;
}
eTMIErrNum CardContents::IsCanSkillLvUp(PktInfoCardGrow* _outPkt, PktInfoProductComGrowReq&& _pktInfo, const bool _ignoreCost) const
{
    const PktInfoCard::Piece& piece{ __GetInst(_pktInfo.targetUID_) };

    // 카드(서포터) 스킬 레벨업 부분 참고 -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/서포터#.EC.8A.A4.ED.82.AC_.EB.A0.88.EB.B2.A8.EC.97.85
    // 비용 계산
    c_uint64 myNowGold{ _GetUserGold() };
    uint64_t lvUpCost{};
    if (false == _ignoreCost)
    {
        // 비용 소모 확인
        lvUpCost        = (TableCard::Inst->GetCardSkillLevelupCostByGrade(piece.GetGrade()) * _pktInfo.GetTotalStackCnt());
        if (myNowGold < lvUpCost)       return eTMIErrNum::COM_ERR_LACK_GOLD;
    }

    // 재료 확인 및 스킬 레벨 계산
    c_uint32 targetTID{ piece.tableID_ };
    uint8_t skillLv{ piece.skillLv_ };
    c_uint8 maxSkillLv{ TableBalance::Inst->Get().CardMaxSkillLevel };
    if (maxSkillLv <= skillLv)          return eTMIErrNum::COM_ERR_ALREADY_MAX_LEVEL;

    auto materWork      = [&targetTID, &skillLv](const PktInfoCard::Piece& _mater) -> eTMIErrNum {

                        if (targetTID != _mater.tableID_)   return eTMIErrNum::SVR_CARD_LV_ERR_NOT_SAME_TYPE_MATERIAL;

                        skillLv     += _mater.skillLv_;
                        return eTMIErrNum::SUCCESS_OK;
                    };
    c_uint8 chkGrade{ piece.GetGrade() };
    auto itemWork       = [&skillLv, &chkGrade](const PktInfoItem::Piece& _piece, c_uint16 _cnt) -> eTMIErrNum {
                        
                        if (false == _piece.IsCanSkillLvUp(chkGrade) || false == _piece.IsCanUseMaterial(eITEMSUBTYPE::MATERIAL_CARD_SLVUP))
                            return eTMIErrNum::COM_ERR_LV_OTHER_TYPE_MATERIAL;

                        skillLv     += static_cast<c_uint8>(_piece.GetLvFromValue() * _cnt);
                        return eTMIErrNum::SUCCESS_OK;
                    };
    eTMIErrNum err{ __IsCheckCommonGrow(&_outPkt->comGrow_, _pktInfo, piece, materWork, itemWork) };
    if (eTMIErrNum::SUCCESS_OK != err)  return err;

    if (maxSkillLv < skillLv)
        skillLv         = maxSkillLv;

    PktInfoProductComGrowAck* outComGrow{ &_outPkt->comGrow_ };
    outComGrow->DoMoveInit(MOVE_CAST(_pktInfo));
    outComGrow->expLv_.DoSetExpLv(piece.exp_, piece.lv_);
    outComGrow->targetUID_  = _pktInfo.targetUID_;
    outComGrow->userGold_   = myNowGold - lvUpCost;
    _outPkt->retWake_       = piece.wake_;
    _outPkt->retSkillLv_    = skillLv;
    return err;
}
eTMIErrNum CardContents::IsCanWake(PktInfoCardGrow* _outPkt, PktInfoProductComGrowReq&& _pktInfo, STMaxLenFmtString* _logMsg, const bool _ignoreCost) const
{
    const PktInfoCard::Piece& piece{ __GetInst(_pktInfo.targetUID_) };
    if (true == piece.IsNull())             return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;
    const DataCard& data{ piece.Table() };
    if (true == data.IsNull())              return eTMIErrNum::COM_ERR_NOT_EXIST_TABLE_DATA;
    if (true == piece.IsMaxWake())          return eTMIErrNum::COM_ERR_ALREADY_MAX_WAKE_VALUE;
    if (false == piece.IsMaxLv())           return eTMIErrNum::COM_ERR_NOT_MAX_LV;
    if (true == _pktInfo.IsExistMater())    return eTMIErrNum::COM_ERR_NOT_NEED_MATERIAL_SETTING;

    PktInfoProductComGrowAck* outComGrow{ &_outPkt->comGrow_ };

    // 카드(서포터) 각성 부분 참고 -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/서포터#.EA.B0.81.EC.84.B1
    // 비용 계산
    eTMIErrNum err{ eTMIErrNum::SUCCESS_OK };
    c_uint64 myNowGold{ _GetUserGold() };
    uint64_t wakeCost{};
    if (false == _ignoreCost)
    {
        // 각성 재료 테이블 정보 검증
        const DataItemReqList& dataItemReq{ TableItemReqList::Inst->GetData(data.WakeReqGroup, piece.wake_) };
        if (true == dataItemReq.IsNull())   return eTMIErrNum::COM_ERR_NOT_EXIST_ITEMREQLIST_NEED_WAKE;

        // 무기 각성 비용 소모 설정 및 확인
        wakeCost        = dataItemReq.Gold;
        if (myNowGold < wakeCost)           return eTMIErrNum::COM_ERR_LACK_GOLD;

        // 각성 재료 존재 여부 및 수량 확인 후 설정
        err             = _GetUser()->IsCheckContumeItemByReqTable(&outComGrow->materItems_, dataItemReq, _MAC_FUNC_);
        if (err != eTMIErrNum::SUCCESS_OK)
        {
            piece.GetLogStr(_logMsg);
            dataItemReq.GetLogStr(_logMsg);
            return err;
        }
    }

    // 각성이 되면 레벨을 1로 초기화 합니다.
    outComGrow->expLv_.DoSetExpLv(0, 1);
    outComGrow->targetUID_  = _pktInfo.targetUID_;
    outComGrow->userGold_   = myNowGold - wakeCost;
    _outPkt->retWake_       = piece.wake_ + 1;
    _outPkt->retSkillLv_    = piece.skillLv_;
    return err;
}
eTMIErrNum CardContents::IsCanChangeExpInFavor(PktInfoCardBook::Piece* _outPkt, c_uint32 _charTID, c_int32 _gapExp, c_tchar* _msg, const bool _maxErr) const
{
    // 경험치가 0일 경우는 아무 처리도 하지 않습니다.
    if (INVALID_VALUE == _gapExp)       return eTMIErrNum::SUCCESS_OK;

    const PktInfoCardBook::Piece& piece{ __GetBookInst(_charTID, _msg) };
    if (true == piece.IsNull())         return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;

    (*_outPkt)          = piece;

    PktInfoExpLv expLv;
    expLv.DoSetExpLv(_outPkt->favorExp_, _outPkt->favorLv_);
    c_uint8 maxFavorLv{ TableBalance::Inst->Get().CardFavorMaxLevel };
    if (maxFavorLv <= expLv.lv_)
    {
        // 최대 레벨이 에러로 확인해야할 상황이면 에러 값으로 반환해 줍니다.
        if (true == _maxErr)
            return eTMIErrNum::SVR_CARD_LV_ERR_ALREADY_LIMIT_VALUE;
        return eTMIErrNum::SUCCESS_OK;
    }

    // 카드(서포터) 경험치 및 레벨을 확인 후 알맞게 얻어옵니다.
    const eTMIErrNum err{ expLv.DoCheckAddExpAndGetLvExp(piece.Table().FavorGroup, maxFavorLv, expLv.lv_, expLv.exp_, _gapExp) };
    if (eTMIErrNum::SUCCESS_OK != err)
        return err;

    // 카드(서포터) 도감 위키 링크 -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/도감
    // 이번에 최대 레벨(최대 각성이 적용된) 시점이고 아직 도감에 관련 값이 활성화 되지 않은 상태라면 활성화 관련 데이타도 추가합니다.
    if (maxFavorLv <= expLv.lv_ && false == _outPkt->IsOnFlag(PktInfoCardBook::Piece::STATE::MAX_FAVOR_LV))
        _outPkt->DoOnFlag(PktInfoCardBook::Piece::STATE::MAX_FAVOR_LV);

    _outPkt->favorExp_      = expLv.exp_;
    _outPkt->favorLv_       = expLv.lv_;
    _outPkt->changeFlag_    = true;
    return err;
}
eTMIErrNum CardContents::IsCanBookNewConfirm(PktInfoBookNewConfirm* _inOutPkt) const
{
    const PktInfoCardBook::Piece& piece{ __GetBookInst(_inOutPkt->bookTID_, _MAC_FUNC_) };
    if (true == piece.IsNull())         return eTMIErrNum::SVR_BOOK_CARD_ERR_NOT_EXIST_INST;
    if (true == piece.IsOnFlag(PktInfoCardBook::Piece::STATE::NEW_CHK)) return eTMIErrNum::SVR_BOOK_COM_ALREADY_CONFIRM;

    _inOutPkt->retStateFlag_    = piece.stateFlag_;
    DO_ON_IDX_BIT(_inOutPkt->retStateFlag_, PktInfoCardBook::Piece::STATE::NEW_CHK);
    return eTMIErrNum::SUCCESS_OK;
}
eTMIErrNum CardContents::IsCanGetFavorReward(PktInfoBookStateReward* _outPkt, const PktInfoBookOnStateReq& _pktInfo) const
{
    const PktInfoCardBook::Piece& book{ __GetBookInst(_pktInfo.tid_, _MAC_FUNC_) };
    if (true == book.IsNull())          return eTMIErrNum::SVR_BOOK_CARD_ERR_NOT_EXIST_INST;

    const PktInfoCardBook::Piece::STATE idx{ static_cast<PktInfoCardBook::Piece::STATE>(_pktInfo.stateIdx_) };
    if (idx < PktInfoCardBook::Piece::_START_FAVOR_RWD_ || PktInfoCardBook::Piece::_END_FAVOR_RWD_ <= idx)
        return eTMIErrNum::COM_ERR_INVALID_SCOPE_INDEX;

    if (true == book.IsOnFlag(idx))     return eTMIErrNum::SVR_CARD_RWD_ERR_ALREADY_FAVOR_GET_REWARD;

    uint8_t rewardLv{};
    switch (idx)
    {
    case PktInfoCardBook::Piece::STATE::FAVOR_RWD_GET_1 : { rewardLv = 1; } break;
    case PktInfoCardBook::Piece::STATE::FAVOR_RWD_GET_2 : { rewardLv = 2; } break;
    case PktInfoCardBook::Piece::STATE::FAVOR_RWD_GET_3 : { rewardLv = 3; } break;
	case PktInfoCardBook::Piece::STATE::FAVOR_RWD_GET_4 : { rewardLv = 4; } break;
	case PktInfoCardBook::Piece::STATE::FAVOR_RWD_GET_5 : { rewardLv = 5; } break;
	default:                            return eTMIErrNum::COM_ERR_INVALID_SCOPE_INDEX;
    }
    if (book.favorLv_ < rewardLv)       return eTMIErrNum::COM_ERR_LACK_LEVEL;

    const DataLevelUp& dataLv{ TableLevelUp::Inst->GetData(book.Table().FavorGroup, rewardLv) };
    if (true == dataLv.IsNull())        return eTMIErrNum::SVR_COM_NOT_EXIST_TABLE_DATA_LEVELUP;

    // 제품군 관련 생성 처리
    {
        eTMIErrNum err{ eTMIErrNum::SUCCESS_OK };
        AutoProductWorker productWork(*_GetUser(), &_outPkt->products_);
        auto work       = [&productWork, &err](const DataRandom& _data, c_uint64 /*_idx*/) {

                        if (eTMIErrNum::SUCCESS_OK != err)  return ;

                        err     = productWork.DoMakeProduct(_data);
                    };
        if (false == TableRandom::Inst->DoWorkAllToGroupDatas(dataLv.GetCardFavorLvUpRwdTID(), work))
            return eTMIErrNum::SVR_CARD_RWD_ERR_INVALID_REWARD_DATA;

        if (eTMIErrNum::SUCCESS_OK != err)  return err;
    }

    uint32_t stateFlag{ book.stateFlag_ };
    DO_ON_IDX_BIT(stateFlag, idx);
    _outPkt->bookState_.tid_        = _pktInfo.tid_;
    _outPkt->bookState_.stateFlag_  = stateFlag;
    return eTMIErrNum::SUCCESS_OK;
}

eTMIErrNum CardContents::DoDel(c_uint64 _uid, c_tchar* _msg)
{
    ContItr find{ m_Cont.find(_uid) };
    if (m_End == find)
    {
        STMaxLenFmtString log;
        _DoLog(log.Format(_TX("[%s] -> CardContents::DoDel() Err - uid=[%llu]"), _msg, _uid), LogType::_CRI_);
        return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;
    }

    PktInfoCard::Piece*  piece{ find->second };
    // 위치 적용 해제할 카드(서포터)가 있으면 기존에 따로 담아둔 정보도 제거합니다.
    __DoDelPosValue(piece->posKind_, piece->posValue_, piece->posSlotNum_);
    piece->DoResetPosInfo();

    m_Mgr->DoPushToCardPool(piece);
    m_Cont.erase(find);
    m_End               = m_Cont.end();
    return eTMIErrNum::SUCCESS_OK;
}
eTMIErrNum CardContents::DoSetApplyPosCard(const PktInfoCardApplyPos& _pktInfo, c_tchar* _msg)
{
    PktInfoCard::Piece* piece{ __GetInst(_pktInfo.cardUID_, _msg) };
    // 새로운 위치 슬롯에 적용 전에 기존 위치 슬롯 정보를 제거해줍니다.
    __DoDelPosValue(piece->posKind_, piece->posValue_, piece->posSlotNum_);
    // 새로운 위치 슬롯에 정보를 추가해 줍니다.
    __DoAddPosValue(piece, _pktInfo.posKind_, _pktInfo.posValue_, _pktInfo.slotNum_);
    piece->posValue_    = _pktInfo.posValue_;
    piece->posKind_     = _pktInfo.posKind_;
    piece->posSlotNum_  = _pktInfo.slotNum_;

    // 기존 위치 슬롯에 적용된 카드(서포터)가 있다면 교체 처리를 합니다.
    c_uint64 oldCardUID{ _pktInfo.oldCardUID_ };
    if (INVALID_UID != oldCardUID)
    {
        PktInfoCard::Piece* oldPiece{ __GetInst(oldCardUID, _msg) };
        // 교체든 탈착이든 위치 정보는 초기화 합니다.
        oldPiece->DoResetPosInfo();
        // 탈착 상황이 아니라면 교체기 때문에 교체할 슬롯으로 새로 설정합니다.
        if (false == _pktInfo.IsDetechToOldCardSlot())
        {
            __DoAddPosValue(oldPiece, _pktInfo.posKind_, _pktInfo.posValue_, _pktInfo.oldCardChangeSlotNum_);
            oldPiece->posValue_     = _pktInfo.posValue_;
            oldPiece->posKind_      = _pktInfo.posKind_;
            oldPiece->posSlotNum_   = _pktInfo.oldCardChangeSlotNum_;
        }
    }
    return eTMIErrNum::SUCCESS_OK;
}
eTMIErrNum CardContents::DoSetApplyPosCards(const PktInfoContentsSlotPos& _pktInfo, c_tchar* _msg)
{
    for (const auto& info : _pktInfo.infos_)
    {
        PktInfoCard::Piece* piece{ __GetInst(info.uid_, _msg) };
        if (true == piece->IsNull())        return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;

        // 새로운 위치 슬롯에 정보를 추가해 줍니다.
        __DoAddPosValue(piece, info.kind_, info.value_, info.slotNum_);
        piece->posValue_    = info.value_;
        piece->posKind_     = info.kind_;
        piece->posSlotNum_  = info.slotNum_;
    }
    return eTMIErrNum::SUCCESS_OK;
}
eTMIErrNum CardContents::DoSetApplyOutPosCard(const PktInfoCardApplyOutPos& _pktInfo, c_tchar* _msg)
{
    return __DoDelPosValue(_pktInfo.cardUID_, _msg);
}
eTMIErrNum CardContents::DoSetApplyOutPosCards(const PktInfoUIDList& _pktInfo, c_tchar* _msg)
{
    eTMIErrNum err{ eTMIErrNum::SUCCESS_OK };
    for (const auto& uid : _pktInfo.uids_)
    {
        err             = __DoDelPosValue(uid, _msg);
        if (eTMIErrNum::SUCCESS_OK != err)
            break;
    }
    return err;
}
eTMIErrNum CardContents::DoSetLock(c_uint64 _uid, const bool _lockFlag, c_tchar* _msg)
{
    PktInfoCard::Piece* piece{ __GetInst(_uid, _msg) };
    piece->lock_        = _lockFlag;
    return eTMIErrNum::SUCCESS_OK;
}
eTMIErrNum CardContents::DoSetLvUpInfo(const PktInfoCardGrow& _pktInfo, c_tchar* _msg)
{
    return __DoApplyCommonGrow(_pktInfo, _msg, eAchieveType::AM_Etc_SptUpgrade, _pktInfo.comGrow_.GetTotalMaterCnt(), static_cast<c_uint32>(_pktInfo.growState_));
}
eTMIErrNum CardContents::DoSetSkillLvUpInfo(const PktInfoCardGrow& _pktInfo, c_tchar* _msg)
{
    // 패스 미션(카드(서포터) 스킬 강화)
    AutoPassMissionWorker passWorker(*_GetUser(), ePassMissionType::PM_SLvUP_Cnt, static_cast<c_uint32>(eContentsPosKind::CARD), 1, _MAC_FUNC_);
    return __DoApplyCommonGrow(_pktInfo, _msg, eAchieveType::AM_Etc_SptSLvUP, _pktInfo.comGrow_.GetTotalMaterCnt());
}
eTMIErrNum CardContents::DoSetWakeInfo(const PktInfoCardGrow& _pktInfo, c_tchar* _msg)
{
    // 패스 미션(카드(서포터) 각성)
    AutoPassMissionWorker passWorker(*_GetUser(), ePassMissionType::PM_WakeUP_Cnt, static_cast<c_uint32>(eContentsPosKind::CARD), 1, _MAC_FUNC_);
    return __DoApplyCommonGrow(_pktInfo, _msg, eAchieveType::AM_Etc_SptWake);
}
eTMIErrNum CardContents::DoSetBookState(c_uint32 _tid, c_uint32 _stateFlag, c_tchar* _msg)
{
    PktInfoCardBook::Piece* piece{ __GetBookInst(_tid, _msg) };
    if (true == piece->IsNull())         return eTMIErrNum::SVR_BOOK_CARD_ERR_NOT_EXIST_INST;

    piece->stateFlag_   = _stateFlag;
    return eTMIErrNum::SUCCESS_OK;
}

eTMIErrNum CardContents::DoSetGameResultInfo(const PktInfoStageGameResultAck& _pktInfo, c_tchar* _msg)
{
    this->DoUpdateBook(_pktInfo.cardBook_, _msg);
    return eTMIErrNum::SUCCESS_OK;
}
eTMIErrNum CardContents::DoUpdateBook(const PktInfoCardBook& _pktInfo, c_tchar* _msg)
{
    // AutoAchieveCondiWorker 매번 생성하지 않고 하나만 생성 후 활용하려고 _val 값을 0넣습니다.
    AutoAchieveCondiWorker achieveWorker(*_GetUser(), eAchieveType::AM_Gro_SptFavor, 0, 0, _MAC_FUNC_);

    const PktInfoCardBook::Pack& bookPack{ _pktInfo.infos_ };
    size_t size{ bookPack.size() };
    for (size_t loop{}; loop < size; ++loop)
    {
        const PktInfoCardBook::Piece& bookInst{ bookPack[loop] };
        PktInfoCardBook::Piece* setBook{ __GetBookInst( bookInst.tableID_, _msg ) };

        // 유저 공적(서포터 호감도 달성 횟수)
        c_uint8 retLv{ bookInst.favorLv_ };
        if (setBook->favorLv_ < retLv)
            achieveWorker.DoWork(eAchieveType::AM_Gro_SptFavor, retLv, 1, _MAC_FUNC_);
        (*setBook)      = bookInst;
    }
    return eTMIErrNum::SUCCESS_OK;
}

eTMIErrNum CardContents::DoMakeProduct(PktInfoCard* _outPkt, PktInfoCardBook* _outPktBook, c_uint32 _cardTID, const PktInfoProductPack* _checkPkt, bool _lock) const
{
    PktInfoCard::Piece info{};
    info.tableID_       = _cardTID;
    const DataCard& data{ info.Table() };
    if (true == data.IsNull())          return eTMIErrNum::SVR_COM_NOT_EXIST_TABLE_DATA_CARD;

    info.lock_          =   _lock;

    _outPkt->infos_.push_back(info);

    // 카드(서포터) 도감 등록 확인
    if (false == this->IsExistCardBook(_cardTID))
    {
        if (false == _checkPkt->IsExistInInsertExpectCardBook(_cardTID))
        {
            PktInfoCardBook::Piece bookPiece;
            bookPiece.tableID_  = _cardTID;
            _outPktBook->infos_.push_back(bookPiece);
        }
    }
    return eTMIErrNum::SUCCESS_OK;
}

uint64_t CardContents::GetLvUpCost(const PktInfoCard::Piece& _piece, c_uint64 _matCnt) const
{
    // 카드(서포터) 강화 부분 참고 -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/서포터
    c_uint64 cost{ TableBalance::Inst->Get().CardLevelupCostByLevel * _piece.lv_ * _matCnt };
    return cost;
}

const PktInfoCard::Piece& CardContents::GetInst(c_uint64 _uid) const
{
    return __GetInst(_uid);
}
const CardContents::InstVec& CardContents::GetInstVecByApplyPos(const eContentsPosKind _kind, c_uint64 _posValue) const
{
    return __GetInstVecToPosValue(_kind, _posValue);
}
const PktInfoCard::Piece& CardContents::GetInstByApplyPos(const eContentsPosKind _kind, c_uint64 _posValue, c_uint8 _slotNum) const
{
    return __GetInstToPosValue(_kind, _posValue, _slotNum);
}
bool CardContents::IsExistCard(c_uint64 _uid) const
{
    if (true == __GetInst(_uid, false).IsNull())   return false;

    return true;
}
bool CardContents::IsExistCardBook(c_uint32 _cardTID) const
{
    if (true == __GetBookInst(_cardTID, _MAC_FUNC_, false).IsNull())   return false;

    return true;
}
eTMIErrNum CardContents::IsCheckUsing(c_uint64 _uid) const
{
    const PktInfoCard::Piece& piece{ __GetInst(_uid, false) };
    if (true == piece.IsNull())             return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;

    return __IsCheckUseKind(piece);
}

PktInfoCard::Piece* CardContents::__GetInst(c_uint64 _uid, c_tchar* _msg, const bool _logFlag)
{
    const ContItr find{ m_Cont.find(_uid) };
    if (m_End == find)
    {
        if (true == _logFlag)
        {
            STMaxLenFmtString log;
            _DoLog(log.Format(_TX("[%s] -> CardContents::__GetInst() Err - uid=[%llu]"), _msg, _uid), LogType::_CRI_);
        }
        return &PktInfoCard::Piece::ms_Null;
    }
    return find->second;
}
const PktInfoCard::Piece& CardContents::__GetInst(c_uint64 _uid, const bool _logFlag) const
{
    return *const_cast<CardContents*>(this)->__GetInst(_uid, _MAC_FUNC_, _logFlag);
}
PktInfoCardBook::Piece* CardContents::__GetBookInst(c_uint32 _cardTID, c_tchar* _msg, const bool _logFlag)
{
    BookContItr find{ m_BookCont.find(_cardTID) };
    if (m_BookEnd == find)
    {
        if (true == _logFlag)
        {
            STMaxLenFmtString log;
            _DoLog(log.Format(_TX("[%s] -> CardContents::__GetBookInst() Err - tid=[%u]"), _msg, _cardTID), LogType::_CRI_);
        }
        return &PktInfoCardBook::Piece::ms_Null;
    }
    return find->second;
}
const PktInfoCardBook::Piece& CardContents::__GetBookInst(c_uint32 _cardTID, c_tchar* _msg, const bool _logFlag) const
{
    return *const_cast<CardContents*>(this)->__GetBookInst(_cardTID, _msg, _logFlag);
}

CardContents::InstVec* CardContents::__GetInstVecToPosValue(const eContentsPosKind _kind, c_uint64 _posValue)
{
    const auto kind{ static_cast<c_uint8>(_kind) };
    if (m_PosContVec.size() <= kind)    return &ms_InstVec;

    const PosCont& posCont{ m_PosContVec[kind] };
    const PosContItrC find{ posCont.find(_posValue) };
    if (posCont.end() == find)          return &ms_InstVec;

    return find->second;
}
const CardContents::InstVec& CardContents::__GetInstVecToPosValue(const eContentsPosKind _kind, c_uint64 _posValue) const
{
    return *const_cast<CardContents*>(this)->__GetInstVecToPosValue(_kind, _posValue);
}
const PktInfoCard::Piece& CardContents::__GetInstToPosValue(const eContentsPosKind _kind, c_uint64 _posValue, c_uint8 _slotNum) const
{
    if (eCardSlotPosMax::_SLOT_MAX_ <= _slotNum)
    {
        STMaxLenFmtString log;
        log.Format(_TX("CardContents::__GetInstToPosValue Err - (MAX[%u] <= _slotNum[%u]) - CardUID[%llu]"), eCardSlotPosMax::_SLOT_MAX_, _slotNum);
        _DoLog(log.Get(), LogType::_ERR_);
        return PktInfoCard::Piece::ms_Null;
    }
    const InstVec& instVec{ __GetInstVecToPosValue(_kind, _posValue) };
    if (true == instVec.empty())        return PktInfoCard::Piece::ms_Null;

    return *instVec[_slotNum];
}

eTMIErrNum CardContents::__IsCheckUseKind(const PktInfoCard::Piece& _pkt) const
{
    eTMIErrNum err{ eTMIErrNum::SUCCESS_OK };
    const eContentsPosKind kind{ _pkt.posKind_ };
    if (eContentsPosKind::_NONE_ != kind)
    {
        switch (kind)
        {
        case eContentsPosKind::CHAR:        { err   = eTMIErrNum::COM_ERR_POS_ERR_APPLY_CHARACTER; } break;
        case eContentsPosKind::FACILITY:    { err   = eTMIErrNum::COM_ERR_POS_ERR_APPLY_FACILITY; } break;
        default:                            { err   = eTMIErrNum::COM_ERR_POS_ERR_APPLY_WHERE; } break;
        }
    }
    return err;
}
eTMIErrNum CardContents::__IsCheckCanSell(c_uint64 _uid, const PktInfoCard::Piece** _outPiece) const
{
    const PktInfoCard::Piece& piece{ __GetInst(_uid) };
    if (true == piece.IsNull())             return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;
    if (true == piece.lock_)                return eTMIErrNum::COM_ERR_DONT_WORK_TO_LOCKED;
    
    const DataCard& data{ piece.Table() };
    if (true == data.IsNull())              return eTMIErrNum::COM_ERR_NOT_EXIST_TABLE_DATA;

    (*_outPiece)        = &piece;
    return __IsCheckUseKind(piece);
}
eTMIErrNum CardContents::__IsCheckCommonGrow(PktInfoProductComGrowAck* _outPkt, const PktInfoProductComGrowReq& _reqPkt, const PktInfoCard::Piece& _piece, LAMBDA(eTMIErrNum(const PktInfoCard::Piece& _mater), _matWork), LAMBDA(eTMIErrNum(const PktInfoItem::Piece& _piece, c_uint16 _cnt), _itemWork), c_uint8 _maxMaterCnt) const
{
    if (true == _piece.IsNull())                return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;
    const DataCard& data{ _piece.Table() };
    if (true == data.IsNull())                  return eTMIErrNum::COM_ERR_NOT_EXIST_TABLE_DATA;

    const PktInfoUIDList::Pack& maters{ _reqPkt.maters_.uids_ };
    c_size_t materSize{ maters.size() };
    size_t totalMaterSize{ materSize };

    const PktInfoItemCntVec& materItemVec{ _reqPkt.materItems_ };
    totalMaterSize      += materItemVec.GetTotalStackCnt();

    // 재료가 하나도 없을 경우
    if (0 == totalMaterSize)                    return eTMIErrNum::COM_ERR_EMPTY_NEED_MATERIAL;
    // 재료 개수가 처리 가능한 최대 개수보다 많은지 확인합니다.
    if (_maxMaterCnt < static_cast<c_uint8>(totalMaterSize))   return eTMIErrNum::COM_ERR_OVER_REQUEST_COUNT;

    c_uint64 targetUID{ _piece.cardUID_ };
    eTMIErrNum err{ eTMIErrNum::SUCCESS_OK };
    for (size_t loop{}; loop < materSize; ++loop)
    {
        c_uint64 matUID{ maters[loop] };
        // 현재 재료가 다시 한번 재료 또 있는지 검증
        for (size_t checkLoop{loop + 1}; checkLoop < materSize; ++checkLoop)
        {
            if (matUID == maters[checkLoop])
                return eTMIErrNum::COM_ERR_DUPLICATE_MATERIAL;
        }
        const PktInfoCard::Piece* materPiece{};
        err             = __IsCheckCanSell(matUID, &materPiece);
        if (eTMIErrNum::SUCCESS_OK != err)
        {
            if (eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST == err)         { err   = eTMIErrNum::SVR_CARD_LV_ERR_NOT_EXIST_MATERIAL; }
            else if (eTMIErrNum::COM_ERR_DONT_WORK_TO_LOCKED == err)        { err   = eTMIErrNum::COM_ERR_LV_LOCKED_MATERIAL; }
            else if (eTMIErrNum::COM_ERR_POS_ERR_APPLY_CHARACTER == err)    { err   = eTMIErrNum::COM_ERR_LV_USING_CHARACTER_MATERIAL; }
            else if (eTMIErrNum::COM_ERR_NOT_EXIST_TABLE_DATA == err)       { err   = eTMIErrNum::COM_ERR_LV_NOT_EXIST_TABLE_DATA_MATERIAL; }

            return err;
        }
        if (targetUID == materPiece->cardUID_)  return eTMIErrNum::COM_ERR_EIXST_TARGET_IN_MATERIAL;

        err             = _matWork(*materPiece);
        if (eTMIErrNum::SUCCESS_OK != err)      return err;
    }

    // 제료 아이템 확인
    err                 = _IsCheckItemWhenGetResult(&_outPkt->materItems_, materItemVec, _itemWork, _MAC_FUNC_);
    return err;
}
eTMIErrNum CardContents::__IsCanChangeExp(PktInfoCardBook* _outPktBook, PktInfoExpLv* _outRetExpLv, const PktInfoCard::Piece& _piece, c_int32 _gapExp, const bool _maxErr) const
{
    if (0 == _gapExp)                   return eTMIErrNum::SVR_CARD_LV_ERR_CHANGE_GAQ_LV_ZERO;
    if (true == _piece.IsNull())        return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;

    const DataCard& data{ _piece.Table() };
    if (true == data.IsNull())          return eTMIErrNum::COM_ERR_NOT_EXIST_TABLE_DATA;
    
    c_uint8 maxLv{ _piece.GetMaxLevel() };
    if (maxLv <= _piece.lv_)
    {
        _outRetExpLv->DoSetExpLv(_piece.exp_, _piece.lv_, true);
        if (true == _maxErr)
            return eTMIErrNum::SVR_CARD_LV_ERR_ALREADY_LIMIT_VALUE;
        return eTMIErrNum::SUCCESS_OK;
    }

    // 카드(서포터) 경험치 및 레벨을 확인 후 알맞게 얻어옵니다.
    const eTMIErrNum err{ _outRetExpLv->DoCheckAddExpAndGetLvExp(data.LevelUpGroup, maxLv, _piece.lv_, _piece.exp_, _gapExp) };
    if (eTMIErrNum::SUCCESS_OK != err)
        return err;

    // 카드(서포터) 도감 위키 링크 -> http://10.10.10.90/Mediawiki/index.php/Project-A/일반/도감
    // 이번에 최대 레벨(최대 각성이 적용된) 시점이고 아직 도감에 관련 값이 활성화 되지 않은 상태라면 활성화 관련 데이타도 추가합니다.
    if (maxLv <= _outRetExpLv->lv_ && true == _piece.IsMaxWake()
        && false == _piece.book_->IsOnFlag(PktInfoCardBook::Piece::STATE::MAX_WAKE_AND_LV))
    {
        PktInfoCardBook::Piece bookPiece{ *_piece.book_ };
        bookPiece.DoOnFlag(PktInfoCardBook::Piece::STATE::MAX_WAKE_AND_LV);
        _outPktBook->infos_.push_back(bookPiece);
    }
    return err;
}

void CardContents::__DoAddPosValue(const PktInfoCard::Piece* _instCard, const eContentsPosKind _kind, c_uint64 _posValue, c_uint8 _posSlotNum)
{
    const auto kind{ static_cast<c_uint8>(_kind) };
    if (m_PosContVec.size() <= kind)    return ;

    InstVec* posInstVec{};
    PosCont& posCont{ m_PosContVec[kind] };
    PosContItr find{ posCont.find(_posValue) };
    if (posCont.end() == find)
    {
        posInstVec      = _new(InstVec);
        m_PosContVec[kind].insert(PosCont::value_type(_posValue, posInstVec));
        posInstVec->resize(eCardSlotPosMax::_SLOT_MAX_);
        for (uint64_t loop{}; loop < eCardSlotPosMax::_SLOT_MAX_; ++loop)
            (*posInstVec)[loop] = &PktInfoCard::Piece::ms_Null;
    }
    else
        posInstVec      = find->second;

    if (eCardSlotPosMax::_SLOT_MAX_ <= _posSlotNum)
    {
        STMaxLenFmtString log;
        log.Format(_TX("CardContents::__DoAddPosValue Err - (MAX[%u] <= _posSlotNum[%u]) - CardUID[%llu]"), eCardSlotPosMax::_SLOT_MAX_, _posSlotNum, _instCard->cardUID_);
        _DoLog(log.Get(), LogType::_ERR_);
        return ;
    }
    (*posInstVec)[_posSlotNum]   = _instCard;
}
eTMIErrNum CardContents::__DoDelPosValue(c_uint64 _uid, c_tchar* _msg)
{
    PktInfoCard::Piece* piece{ __GetInst(_uid, _msg) };
    if (true == piece->IsNull())        return eTMIErrNum::SVR_CARD_COM_ERR_NOT_EXIST_INST;

    // 위치 적용 해제할 카드(서포터)의 기존에 따로 담아둔 정보도 제거합니다.
    __DoDelPosValue(piece->posKind_, piece->posValue_, piece->posSlotNum_);
    piece->DoResetPosInfo();
    return eTMIErrNum::SUCCESS_OK;
}
void CardContents::__DoDelPosValue(const eContentsPosKind _kind, c_uint64 _posValue, c_uint8 _posSlotNum)
{
    if (eCardSlotPosMax::_SLOT_MAX_ <= _posSlotNum)
        return ;

    const auto kind{ static_cast<c_uint8>(_kind) };
    if (_kind <= eContentsPosKind::_NONE_ || m_PosContVec.size() <= kind)    return ;

    PosCont& posCont{ m_PosContVec[kind] };
    PosContItr find{ posCont.find(_posValue) };
    if (posCont.end() == find)
        return ;

    InstVec* posInstVec{ find->second };
    (*posInstVec)[_posSlotNum]  = &PktInfoCard::Piece::ms_Null;
}

eTMIErrNum CardContents::__DoApplyCommonGrow(const PktInfoCardGrow& _pktInfo, c_tchar* _msg, const eAchieveType _achieveTP, c_uint32 _addVal, c_uint32 _achieveIdx)
{
    const PktInfoProductComGrowAck& comGrow{ _pktInfo.comGrow_ };
    const PktInfoExpLv& expLv{ comGrow.expLv_ };
    PktInfoCard::Piece* piece{ __GetInst(comGrow.targetUID_, _msg) };

    // 유저 공적(서포터 레벨업, 스킬 강화, 각성 성공)
    AutoAchieveCondiWorker achieveWorker(*_GetUser(), _achieveTP, _achieveIdx, _addVal, _MAC_FUNC_);
    // 유저 공적(서포터 특정 레벨 달성 횟수)
    c_uint8 retLv{ expLv.lv_ };
    if (piece->lv_ != retLv)
        achieveWorker.DoWork(eAchieveType::AM_Gro_SptLv, retLv, 1, _MAC_FUNC_);

    piece->exp_         = expLv.exp_;
    piece->lv_          = retLv;
    piece->wake_        = _pktInfo.retWake_;
    piece->skillLv_     = _pktInfo.retSkillLv_;

    this->DoUpdateBook(_pktInfo.bookState_, _msg);
    return eTMIErrNum::SUCCESS_OK;
}