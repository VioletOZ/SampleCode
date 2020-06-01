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
// ItemDataGateway�� �̿��� �ε�
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Item::LoadItem(const ItemDataGateway& itemDataGateway)
{
	TRACE;

	// template id�� �ùٸ���, ���ø� ������ �ִ��� üũ
	const ItemTemplate* itemTemplate = GData<ItemDataSheet>()->Find(itemDataGateway.mTemplateId);
	if (nullptr == itemTemplate)
		return false;

	// ���ø� ���� ����
	mItemTemplate = itemTemplate;
	// ������ ������ ����
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

	// ���� ���
	CalcStats();

	return true;

	TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ItemData�� �̿��� �ε�
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Item::LoadItem(const ItemData& itemData)
{
    TRACE;

    // template id�� �ùٸ���, ���ø� ������ �ִ��� üũ
    const ItemTemplate* itemTemplate = GData<ItemDataSheet>()->Find(itemData.mTemplateId);
    if (nullptr == itemTemplate)
        return false;

    // ���ø� ���� ����
    mItemTemplate = itemTemplate;
    // ������ ������ ����
	if (ItemType::EQUIP == itemTemplate->GetType())
	{
		mItemDataBase = xnew<ItemDataEquip>();
		memcpy_s(mItemDataBase, sizeof(ItemData), &itemData, sizeof(ItemData));
	}
	else
	{
		mItemDataBase = xnew<ItemData>(itemData);
	}
    
	// ���� ���
	CalcStats();

	return true;

    TRACE_END;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ������ ���� �� ���� ���� ������ ã�Ƽ� ��ȯ�Ѵ�
// ����)
//  ���� �����ۿ� ���� ���� Ż���ϰų�, Passivity�� �ο��Ǹ� �װ͵鵵 ���縦 �ؾ� �Ѵ�
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatOrder Item::GetMinStatOrder() const
{
    TRACE;

    StatOrder minStatOrder = STAT_ORDER_MAX;

    minStatOrder = mStatPairContainer.GetMinStatOrder();
    if (minStatOrder <= STAT_ORDER_1)
    {
        // ���� ���� ������ ã������ ���̻� ã�� �ʿ䰡 ����.
        return minStatOrder;
    }

    // TODO : ���� �����ۿ� ���� ���� Ż���ϰų�, Passivity�� �ο��Ǹ� �װ͵鵵 ���縦 �ؾ� �Ѵ�

    return minStatOrder;

    TRACE_END;
}

void Item::CalcStats()
{
	// ������ ������ ������ ���� �̷����� �Ʒ� ���� ����� ������.
// ��� �������� ��쿡�� ������ ��� �ݿ��Ͽ� ���Ȱ��
	if (ItemType::EQUIP != mItemTemplate->GetType()) return;

	StatPairList statPairList;

	const ItemEquipDetail* equipDetail = mItemTemplate->GetEquipDetail();
	_ASSERT_CRASH(equipDetail);  // ���!

	// ���Ⱥ���
	const StatPairList& itemStatList = equipDetail->GetStatList();
	for (const StatPair& statPair : itemStatList)
		statPairList.push_back(statPair);

	// ���� ���� �����̳�, Ư����ȭ ������ ���� �߰� ������ ���⿡�� �����ָ� �ȴ�

	mStatPairContainer.Setup(std::move(statPairList));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ItemCount Item::SetCount(ItemCount count)
{
	TRACE;

	_ASSERT_CRASH(nullptr != mItemDataBase);

	// ���� ������ �������̾�� �Ѵ�
	_ASSERT_DEBUG(IsStackable());

	ItemCount newStackCount = count;
	const ItemCount maxStackCount = GetMaxStackCount();
	ItemCount remainStackCount = 0;

	if (newStackCount > maxStackCount)
		remainStackCount = newStackCount - maxStackCount;

	// Over/Under-Flow ����
	newStackCount = std::clamp(newStackCount, (ItemCount)0, maxStackCount);

	mItemDataBase->mStackCount = newStackCount;

	// 0�� ���� ī��Ʈ�� ������ �� ����
	// �� �Լ��� ȣ���ϱ� ������ ���� üũ�� �Ͽ� ������ �ϴ� �� �´�
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

    // ���� �� ��ø �ִ� ������ �۾����� ������ ���� �� ������,
    // ������ ������ �ʵ��� clamping ����� �Ѵ�
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

	// ���� ���
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

	// ���� ���
	CalcStats();

	return true;

	TRACE_END;
}
