#include "Item.h"

#include "ServerCommon/Datasheet/Sheet/ItemDataSheet.h"
#include "ServerCommon/Datasheet/Template/ItemEquipDetail.h"
#include "GameObject/User.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Item::Item(InventoryComponent* ownerInventory)
    :
    mOwnerInventory(ownerInventory)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Item::~Item()
{
	if (nullptr != mItemDataBase) {
		xdelete(mItemDataBase);
		mItemDataBase = nullptr;
	}		
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ItemDataGateway를 이용해 로드
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Item::LoadItem(const ItemDataGateway& itemDataGateway)
{
	TRACE;

	// template id가 올바른지, 템플릿 파일이 있는지 체크
	const ItemTemplate* itemTemplate = GData<ItemDataSheet>()->Find(itemDataGateway.mTemplateId);
	if (nullptr == itemTemplate)
		return false;

	// 템플릿 정보 저장
	mItemTemplate = itemTemplate;
	// 아이템 데이터 저장
	if (ItemType::EQUIP == itemTemplate->GetType())
	{
		mItemDataBase = xnew<ItemDataEquip>();
		memcpy_s(mItemDataBase, sizeof(ItemDataEquip), &itemDataGateway, sizeof(ItemDataGateway));
	}
	else
	{
		mItemDataBase = xnew<ItemData>();
		memcpy_s(mItemDataBase, sizeof(ItemData), &itemDataGateway, sizeof(ItemData));
	}

	// 스탯 계산
	CalcStats();

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ItemData를 이용해 로드
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Item::LoadItem(const ItemData& itemData)
{
    TRACE;

    // template id가 올바른지, 템플릿 파일이 있는지 체크
    const ItemTemplate* itemTemplate = GData<ItemDataSheet>()->Find(itemData.mTemplateId);
    if (nullptr == itemTemplate)
        return false;

    // 템플릿 정보 저장
    mItemTemplate = itemTemplate;
    // 아이템 데이터 저장
	if (ItemType::EQUIP == itemTemplate->GetType())
	{
		mItemDataBase = xnew<ItemDataEquip>();
		memcpy_s(mItemDataBase, sizeof(ItemData), &itemData, sizeof(ItemData));
	}
	else
	{
		mItemDataBase = xnew<ItemData>(itemData);
	}
    
	// 스탯 계산
	CalcStats();

	return true;

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 아이템 스탯 중 제일 낮은 차수를 찾아서 반환한다
// 참고)
//  이후 아이템에 보석 등을 탈착하거나, Passivity가 부여되면 그것들도 조사를 해야 한다
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatOrder Item::GetMinStatOrder() const
{
    TRACE;

    StatOrder minStatOrder = STAT_ORDER_MAX;

    minStatOrder = mStatPairContainer.GetMinStatOrder();
    if (minStatOrder <= STAT_ORDER_1)
    {
        // 제일 낮은 차수를 찾았으면 더이상 찾을 필요가 없다.
        return minStatOrder;
    }

    // TODO : 이후 아이템에 보석 등을 탈착하거나, Passivity가 부여되면 그것들도 조사를 해야 한다

    return minStatOrder;

    TRACE_END;
}

void Item::CalcStats()
{
	// 아이템 데이터 저장이 먼저 이뤄져야 아래 스탯 계산이 동작함.
// 장비 아이템일 경우에만 아이템 등급 반영하여 스탯계산
	if (ItemType::EQUIP != mItemTemplate->GetType()) return;

	StatPairList statPairList;

	const ItemEquipDetail* equipDetail = mItemTemplate->GetEquipDetail();
	_ASSERT_CRASH(equipDetail);  // 장비만!

	// 스탯부터
	const StatPairList& itemStatList = equipDetail->GetStatList();
	for (const StatPair& statPair : itemStatList)
		statPairList.push_back(statPair);

	// 추후 각종 보석이나, 특수강화 등으로 인한 추가 스탯을 여기에서 더해주면 된다

	mStatPairContainer.Setup(std::move(statPairList));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ItemCount Item::SetCount(ItemCount count)
{
	TRACE;

	_ASSERT_CRASH(nullptr != mItemDataBase);

	// 스택 가능한 아이템이어야 한다
	_ASSERT_DEBUG(IsStackable());

	ItemCount newStackCount = count;
	const ItemCount maxStackCount = GetMaxStackCount();
	ItemCount remainStackCount = 0;

	if (newStackCount > maxStackCount)
		remainStackCount = newStackCount - maxStackCount;

	// Over/Under-Flow 방지
	newStackCount = std::clamp(newStackCount, (ItemCount)0, maxStackCount);

	mItemDataBase->mStackCount = newStackCount;

	// 0개 스택 카운트를 설정할 순 없다
	// 이 함수를 호출하기 이전에 조건 체크를 하여 삭제를 하는 게 맞다
	_ASSERT_DEBUG(mItemDataBase->mStackCount > 0);

	return remainStackCount;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ItemCount Item::ChangeCount(int16 delta)
{
    TRACE;

    ItemCount newStackCount = mItemDataBase->mStackCount + delta;
    return SetCount(newStackCount);

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ItemCount Item::GetRemainStackCount() const
{
    TRACE;

    // 서비스 중 중첩 최대 개수가 작아지면 음수로 나올 수 있으니,
    // 음수가 나오지 않도록 clamping 해줘야 한다
    const ItemCount remainStackCount = mItemTemplate->GetMaxStackCount() - mItemDataBase->mStackCount;
    return std::max(MIN_ITEM_COUNT, remainStackCount);

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Item::UpdateLock(bool locked)
{
    TRACE;

	
	if (nullptr == mItemTemplate || ItemType::EQUIP != mItemTemplate->GetType())
		return false;
	
	ItemDataEquip* itemDataEquip = reinterpret_cast<ItemDataEquip*>(mItemDataBase);

	itemDataEquip->mIsLocked = locked;

    return true;

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Item::UpdateLevel(Level level, ItemGrade grade, Exp exp)
{
	TRACE;

	if (nullptr == mItemTemplate || ItemType::EQUIP != mItemTemplate->GetType())
		return false;

	ItemDataEquip* itemDataEquip = reinterpret_cast<ItemDataEquip*>(mItemDataBase);

	itemDataEquip->mLevel = level;
	itemDataEquip->mGrade = grade;
	itemDataEquip->mExp = exp;

	// 스탯 계산
	CalcStats();

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Item::UpdatePreset(PresetValue presetValue)
{
	TRACE;

	if (nullptr == mItemTemplate || ItemType::EQUIP != mItemTemplate->GetType())
		return false;

	ItemDataEquip* itemDataEquip = reinterpret_cast<ItemDataEquip*>(mItemDataBase);

	itemDataEquip->mPresetValue = presetValue;

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Item::UpdateSocket(ItemDBId parentItemDBId, SocketId socketId)
{
	TRACE;

	if (nullptr == mItemTemplate || ItemType::EQUIP != mItemTemplate->GetType())
		return false;

	ItemDataEquip* itemDataEquip = reinterpret_cast<ItemDataEquip*>(mItemDataBase);

	itemDataEquip->mParentItemDBId = parentItemDBId;
	itemDataEquip->mSocketId = socketId;

	// 스탯 계산
	CalcStats();

	return true;

	TRACE_END;
}
