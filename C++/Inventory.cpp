#include "Inventory.h"

#include "User/User.h"

#include "ServerCommon/Datasheet/Template/ItemTemplate.h"
#include "ServerCommon/Datasheet/Sheet/ItemDataSheet.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Inventory::Inventory(User* owner)
    :
    mOwner(owner)
{
    const UserDBData& userDBData = mOwner->GetUserDBData();
    mInvenSlotMax += userDBData.mInventoryExtendedCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Inventory::~Inventory()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::CacheItemDataList(
	ItemDataGatewayList&& itemDataGatewayList, ItemPresetInfoList& itemPresetInfoList, 
	ItemSocketInfoList& itemSocketInfoList
)
{
    TRACE;

	mItemDataGatewayList = std::move(itemDataGatewayList);
	
	mItemDBIdSet.clear();
	for (const ItemDataGateway& itemDataGateway : mItemDataGatewayList)
	{
		mItemDBIdSet.insert(itemDataGateway.mItemDBId);
	}

	// DB에서 얻어온 프리셋 정보를 아이템의 presetValue 값에 적용한다.
	for (const ItemPresetInfo& itemPresetInfo : itemPresetInfoList)
	{
		PresetValue presetValue = 1 << (itemPresetInfo.mPresetId - MIN_PRESET_ID);
		for (const ItemDBId itemDBId : itemPresetInfo.mEquipItemDBIdList)
		{
			if (INVALID_ITEM_DB_ID == itemDBId)
				continue;

			ItemDataGateway* itemDataGateway = GetItemData(itemDBId);
			if (nullptr == itemDataGateway)
			{
				_ASSERT_CRASH(0);
				continue;
			}
			itemDataGateway->mPresetValue |= presetValue;
		}
	}

	// DB에서 얻어온 소켓 정보를 아이템의 parentItemDBId와 socketId 값에 적용한다.
	for (const ItemSocketInfo& itemSocketInfo : itemSocketInfoList)
	{
		const ItemDBId parentItemDBId = itemSocketInfo.mParentItemDBId;
		const ItemData* parentItemData = GetItemData(parentItemDBId);

		if (nullptr == parentItemData)
		{
			_ASSERT_CRASH(0);
			continue;
		}
		
		for (SocketId socketId = 0; socketId < MAX_SOCKET_COUNT; ++socketId)
		{
			const ItemDBId childItemDBId = itemSocketInfo.mChildItemDBIdArray[socketId];
			if (INVALID_ITEM_DB_ID == childItemDBId) {
				continue;
			}
			ItemDataGateway* childItemData = GetItemData(childItemDBId);
			if (nullptr == childItemData)
			{
				_ASSERT_CRASH(0);
				continue;
			}
			childItemData->mParentItemDBId = parentItemDBId;
			childItemData->mSocketId = socketId;
		}
	}

    mCached.store(true);

    return true;

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ItemDataGateway* Inventory::GetItemData(ItemDBId itemDBId)
{
    TRACE;

    auto ret = 
		std::find_if(mItemDataGatewayList.begin(), mItemDataGatewayList.end(), [itemDBId](const ItemData& itemData)
    {
        return itemData.mItemDBId == itemDBId;
    });
    if (mItemDataGatewayList.end() != ret)
    {
        return &(*ret);
    }

    return nullptr;

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const ItemDataGateway* Inventory::GetItemData(ItemDBId itemDBId) const
{
    TRACE;

    return const_cast<Inventory*>(this)->GetItemData(itemDBId);

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::AddItem(const ItemData& itemData)
{
    TRACE;

    // TODO : 검사할 것이 있다면, 여기서 미리 검사하자


    // 중복 체크
    auto ret = mItemDBIdSet.find(itemData.mItemDBId);
    if (mItemDBIdSet.end() != ret)
    {
        _ASSERT_CRASH(0);
        return false;
    }

	ItemDataGateway itemDataGateway;
	memcpy(&itemDataGateway, &itemData, sizeof(ItemData));

    // 목록에 추가
	mItemDataGatewayList.emplace_back(std::move(itemDataGateway));

    // 검색용 ID 셋에 등록
    mItemDBIdSet.insert(itemData.mItemDBId);

    return true;

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::AddItems(const ItemDataList& itemDataList)
{
	TRACE;

	bool result = true;

	for (const ItemData& itemData : itemDataList)
	{
		if (false == AddItem(itemData)) 
		{
			result = false;
		}
	}

	return result;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::DeleteItem(ItemDBId itemDBId)
{
    TRACE;

    // 있는지 없는지 체크
    auto ret = mItemDBIdSet.find(itemDBId);
    if (ret == mItemDBIdSet.end())
    {
        _ASSERT_CRASH(0);
        return false;
    }

    // 검색용 ID 셋에서 먼저 제거
    mItemDBIdSet.erase(itemDBId);

    // 목록에서 제거
    EraseIf(mItemDataGatewayList, [itemDBId](ItemData& itemData)
    {
        return itemData.mItemDBId == itemDBId;
    });

    return true;

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Inventory::DeleteItems(const ItemDBIdList& itemDBIdList)
{
	TRACE;

	for (ItemDBId itemDBId : itemDBIdList)
	{
		if (false == DeleteItem(itemDBId))
		{
			_ASSERT_DEBUG(0);
		}
	}

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::ValidateIncreaseItems(const ItemDBIdCountPairList& itemDBIdCountPairList) const
{
	TRACE;

	for (const auto& [itemDBId, count] : itemDBIdCountPairList)
	{
		// 있는지 없는지 체크
		const ItemData* itemData = GetItemData(itemDBId);
		if (nullptr == itemData)
		{
			_ASSERT_DEBUG(0);
			return false;
		}
		if (MAX_ITEM_COUNT < itemData->mStackCount + count)
			return false;
	}

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::ValidateDecreaseItems(const ItemDBIdCountPairList& itemDBIdCountPairList) const
{
	TRACE;

	for (const auto& [itemDBId, count] : itemDBIdCountPairList)
	{
		// 있는지 없는지 체크
		const ItemData* itemData = GetItemData(itemDBId);
		if (nullptr == itemData)
		{
			_ASSERT_DEBUG(0);
			return false;
		}

		// stackCount가 0이면 삭제할 목록에 들어가야 한다
		if (itemData->mStackCount <= count)
			return false;
	}

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::ValidateCreateItems(const ItemIdCountPairList& itemIdCountPairList) const
{
	TRACE;

	// 아이템에 생성 전 Gateway에서 체크해야 할 사항을 여기에 추가하자
	itemIdCountPairList;

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::ValidateDeleteItems(const ItemDBIdCountPairList& itemDBIdList) const
{
	TRACE;

	for (const auto& [itemDBId, count] : itemDBIdList)
	{
		// 있는지 없는지 체크
		const ItemData* itemData = GetItemData(itemDBId);
		if (nullptr == itemData)
		{
			_ASSERT_DEBUG(0);
			return false;
		}

		if (itemData->mStackCount != count)
			return false;
	}

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::ValidateItemPresetUpdate(
	PresetId presetId, const ItemDBIdList& equipItemDBIdList, const ItemDBIdList& unequipItemDBIdList, 
	OUT ItemPresetArray& itemPresetArray
) const
{
	TRACE;

	PresetValue presetValue = 1 << (presetId - MIN_PRESET_ID);

	// 업데이트 할 프리셋 정보 초기화
	itemPresetArray.fill(INVALID_ITEM_DB_ID);

	// 현재의 프리셋 정보를 구해온다
	for (const ItemDataGateway& itemData : mItemDataGatewayList)
	{
		// 프리셋이 일치할 경우만 진행하면 된다
		if (0 == (itemData.mPresetValue & presetValue))
			continue;

		const ItemTemplate* itemTemplate = GData<ItemDataSheet>()->Find(itemData.mTemplateId);
		if (nullptr == itemTemplate)
		{
			_ASSERT_CRASH(0);
			return false;
		}

		if (ItemType::EQUIP != itemTemplate->GetType())
			return false;

		ItemCategory itemCategory = itemTemplate->GetCategory();
		if (
			static_cast<ItemCategory>(MAX_EQUIP_MIN) > itemCategory ||
			static_cast<ItemCategory>(MAX_EQUIP_MAX) < itemCategory
			)
		{
			return false;
		}
		
		size_t presetArrayIndex = static_cast<size_t>(itemCategory) - static_cast<size_t>(MAX_EQUIP_MIN);

		if (INVALID_ITEM_DB_ID != itemPresetArray[presetArrayIndex])
			return false;

		// 업데이트 할 프리셋 정보에 추가한다
		itemPresetArray[presetArrayIndex] = itemData.mItemDBId;
	}

	// 해제할 아이템 목록을 검사한다.
	for (ItemDBId itemDBId : unequipItemDBIdList)
	{
		const ItemDataGateway* itemDataGateway = GetItemData(itemDBId);
		if (nullptr == itemDataGateway)
		{
			_ASSERT_CRASH(0);
			return false;
		}

		if (0 == (itemDataGateway->mPresetValue & presetValue))
			return false;

		const ItemTemplate* itemTemplate = GData<ItemDataSheet>()->Find(itemDataGateway->mTemplateId);
		ItemCategory itemCategory = itemTemplate->GetCategory();

		size_t index = static_cast<size_t>(itemCategory) - static_cast<size_t>(MAX_EQUIP_MIN);

		// 착용되어 있지 않으면 
		if (INVALID_ITEM_DB_ID == itemPresetArray[index])
			return false;

		// 업데이트 할 프리셋 정보에서 제외한다
		itemPresetArray[index] = INVALID_ITEM_DB_ID;
	}

	// 착용할 프리셋 정보를 검사한다
	for (ItemDBId itemDBId : equipItemDBIdList)
	{
		const ItemDataGateway* itemDataGateway = GetItemData(itemDBId);
		if (nullptr == itemDataGateway)
		{
			_ASSERT_CRASH(0);
			return false;
		}

		if (0 != (itemDataGateway->mPresetValue & presetValue))
			return false;		

		const ItemTemplate* itemTemplate = GData<ItemDataSheet>()->Find(itemDataGateway->mTemplateId);
		ItemCategory itemCategory = itemTemplate->GetCategory();

		size_t index = static_cast<size_t>(itemCategory) - static_cast<size_t>(MAX_EQUIP_MIN);

		if (INVALID_ITEM_DB_ID != itemPresetArray[index])
			return false;

		// 업데이트 할 프리셋 정보에서 추가한다
		itemPresetArray[index] = itemDBId;
	}

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::ValidateItemSocketUpdate(
	SocketId socketId, ItemDBId itemDBIdParent, ItemDBId itemDBIdChild, OUT ItemSocketInfo& itemSocketInfoUpdate
) const
{
	TRACE;

	const ItemDataGateway* parentItemData = GetItemData(itemDBIdParent);
	if (nullptr == parentItemData)
	{
		_ASSERT_CRASH(0);
		return false;
	}

	const ItemDataGateway* childItemData = GetItemData(itemDBIdChild);
	if (nullptr == childItemData)
	{
		_ASSERT_CRASH(0);
		return false;
	}

	if (INVALID_ITEM_DB_ID != childItemData->mParentItemDBId)
		return false;

	itemSocketInfoUpdate.mParentItemDBId = itemDBIdParent;

	// 업데이트 할 소켓 정보 취합
	for (const ItemDataGateway& itemData : mItemDataGatewayList)
	{
		if (itemData.mParentItemDBId != itemDBIdParent)
			continue;

		// 이미 소켓팅이 되어 있으면 실패
		if(itemData.mSocketId == socketId)
			return false;

		int arrayIndex = itemData.mSocketId - MIN_SOCKET_ID;
		itemSocketInfoUpdate.mChildItemDBIdArray[arrayIndex] = itemData.mItemDBId;
	}

	// 새로 추가할 소켓팅 정보를 삽입
	int arrayIndex = socketId - MIN_SOCKET_ID;
	itemSocketInfoUpdate.mChildItemDBIdArray[arrayIndex] = itemDBIdChild;

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Inventory::UpdateItemLock(ItemDBId itemDBId, bool isLocked)
{
	TRACE;

	ItemDataGateway* itemDataGateway = GetItemData(itemDBId);
	if (nullptr == itemDataGateway)
	{
		_ASSERT_DEBUG(0);
	}
	else
	{
		itemDataGateway->mIsLocked = isLocked;
	}

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Inventory::UpdateItemCount(const ItemDBIdCountPair& itemUpdate)
{
	TRACE;

	ItemData* itemData = GetItemData(itemUpdate.mItemDBId);
	if (nullptr == itemData)
	{
		_ASSERT_DEBUG(0);
	}
	else
	{
		itemData->mStackCount = itemUpdate.mItemCount;
	}

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Inventory::UpdateItemCountList(const ItemDBIdCountPairList& itemList)
{
	TRACE;

	for (const ItemDBIdCountPair& itemData : itemList)
	{
		UpdateItemCount(itemData);
	}

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Inventory::UpdateItemLevel(ItemDBId itemDBId, Level level, ItemGrade grade, Exp exp)
{
	TRACE;

	ItemDataGateway* itemDataGateway = GetItemData(itemDBId);
	if (nullptr == itemDataGateway)
	{
		_ASSERT_DEBUG(0);
	}
	else
	{
		itemDataGateway->mLevel = level;
		itemDataGateway->mGrade= grade;
		itemDataGateway->mExp= exp;
	}

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::UpdateItemPresetInfo(
	PresetId presetId, const ItemDBIdList& equipDBIdList, const ItemDBIdList& unequipDBIdList, 
	OUT ItemDataEquipList& itemDataEquipList
)
{
	TRACE;

	itemDataEquipList.clear();
	itemDataEquipList.reserve(equipDBIdList.size() + unequipDBIdList.size());

	PresetValue presetValue = 1 << (presetId - MIN_PRESET_ID);

	for (ItemDBId itemDBId : equipDBIdList)
	{
		ItemDataGateway* itemDataGateway = GetItemData(itemDBId);
		if (nullptr == itemDataGateway)
		{
			_ASSERT_CRASH(0);
			continue;
		}

		itemDataGateway->mPresetValue |= presetValue;
		itemDataEquipList.emplace_back(*reinterpret_cast<ItemDataEquip*>(itemDataGateway));
	}

	for (ItemDBId itemDBId : unequipDBIdList)
	{
		ItemDataGateway* itemDataGateway = GetItemData(itemDBId);
		if (nullptr == itemDataGateway)
		{
			_ASSERT_CRASH(0);
			continue;
		}

		itemDataGateway->mPresetValue &= ~presetValue;
		itemDataEquipList.emplace_back(*reinterpret_cast<ItemDataEquip*>(itemDataGateway));
	}

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Inventory::UpdateItemSocketInfo(
	ItemDBId childDBIdUpdate, const ItemSocketInfo& itemSocketInfoUpdate, OUT ItemDataEquip& itemDataEquipUpdated
)
{
	TRACE;

	ItemDataGateway* parentItemData = GetItemData(itemSocketInfoUpdate.mParentItemDBId);
	if (nullptr == parentItemData)
	{
		_ASSERT_CRASH(0);
		return false;
	}

	bool success = false;

	for (SocketId socketId = 0; socketId < MAX_SOCKET_COUNT; ++socketId)
	{
		const ItemDBId childItemDBId = itemSocketInfoUpdate.mChildItemDBIdArray[socketId];

		ItemDataGateway* childItemDataGateway = GetItemData(childItemDBId);
		if(nullptr == childItemDataGateway)
		{
			_ASSERT_CRASH(0);
			continue;
		}

		// 업데이트 대상일 경우, 값을 갱신한다.
		if (childItemDBId == childDBIdUpdate)
		{
			// 인벤토리 저장 데이터에 적용
			childItemDataGateway->mParentItemDBId = itemSocketInfoUpdate.mParentItemDBId;
			childItemDataGateway->mSocketId = socketId;

			// 반환 데이터에 적용
			itemDataEquipUpdated.mParentItemDBId = itemSocketInfoUpdate.mParentItemDBId;
			itemDataEquipUpdated.mSocketId = socketId;

			// 여기에서 바로 리턴해도 되지만, 개발단계에서는 리턴하지 말고 무결성 체크를 모두 돌아보자
			success = true;
		}
		// 업데이트 대상이 아이템은 기존 정보와 일치해야 한다
		else if(
			itemSocketInfoUpdate.mParentItemDBId != childItemDataGateway->mParentItemDBId ||
			socketId != childItemDataGateway->mSocketId
			)
		{
			_ASSERT_CRASH(0);
			continue;
		}
	}

	return success;

	TRACE_END;
}