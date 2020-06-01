#pragma once

#include "ServerCommon/Datasheet/Template/ItemTemplate.h"
#include "GameObjectComponent/StatComponent/StatPairContainer.h"

class InventoryComponent;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// World ������ ��� �־�� �� ������ ���� (DB�κ��� �� ���� �����Ͱ� �ֵ� ����)
// �����۰� ��� �и��ϴ� ���� ������ ���䰡 �ʿ���
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Item : public Disposable
{
private:
	ItemData* mItemDataBase = nullptr;				// ������ ����
	InventoryComponent* mOwnerInventory = nullptr;  // �� �������� �� �ִ� �κ��丮    
	const ItemTemplate* mItemTemplate = nullptr;    // ������ ���ø�
	StatPairContainer   mStatPairContainer;         // �����ۿ� �ο��� ������ ���� ������.


public:
	explicit Item(InventoryComponent* ownerInventory);
	~Item();


public:
	// ItemData�� �ݵ�� ��Ȯ�ϰ� ä���� ä ȣ��Ǿ�� �Ѵ�
	// Ư��, ����� ��� ���� ������ �и��ϰ�!
	bool LoadItem(const ItemDataGateway& itemData);

	// ItemData�� �ݵ�� ��Ȯ�ϰ� ä���� ä ȣ��Ǿ�� �Ѵ�
	// Ư��, ����� ��� ���� ������ �и��ϰ�!
	bool LoadItem(const ItemData& itemData);


	// ���� ����
public:
	// ���� ������ ���� ������ ��ȯ. ������ ������ ������ STAT_ORDER_MAX�� ��ȯ.
	StatOrder GetMinStatOrder() const;

private:
	void CalcStats();


	// StackCount ����
public:
	// ���� ī��Ʈ�� �Է��ϰ�, ���� ���� ī��Ʈ�� ���ǹ��� �� ��ȯ (0���� ���ų� Ŭ ���� ���ǹ�)
	ItemCount SetCount(ItemCount count);

	// ���� ī��Ʈ�� �����ϰ�, ���� ���� ī��Ʈ�� ���ǹ��� �� ��ȯ (0���� ���ų� Ŭ ���� ���ǹ�)
	ItemCount ChangeCount(int16 delta);

	// ���� ���� ����
	ItemCount GetRemainStackCount() const;


	// ���� ������ ����
public:
	// ���/������� ���� ����
	bool UpdateLock(bool locked);

	// ����, ���, ����ġ ����
	bool UpdateLevel(Level level, ItemGrade grade, Exp exp);

	// ���� ����
	bool UpdatePreset(PresetValue presetValue);

	// ������ ����
	bool UpdateSocket(ItemDBId parentItemDBId, SocketId socketId);

	
	// ItemData getters
public:	
	const ItemData* GetItemData() const noexcept { return mItemDataBase; }
	const ItemDataEquip* GetItemDataEquip() const noexcept
	{
		if (nullptr == mItemTemplate || ItemType::EQUIP != mItemTemplate->GetType())
			return nullptr;
		else 
			return reinterpret_cast<ItemDataEquip*>(mItemDataBase);
	}
	ItemDBId GetItemDBId() const noexcept { return mItemDataBase->mItemDBId; }
	ItemId GetTemplateId() const noexcept { return mItemDataBase->mTemplateId; }
	UserDBId GetOwnerDBId() const noexcept { return mItemDataBase->mOwnerDBId; }
	ItemCount GetStackCount() const noexcept { return mItemDataBase->mStackCount; }
	bool IsLocked() const noexcept 
	{ 
		_ASSERT_CRASH(ItemType::EQUIP == mItemTemplate->GetType());
		if (nullptr == mItemTemplate || ItemType::EQUIP != mItemTemplate->GetType())
			return false;
		else 
			return reinterpret_cast<ItemDataEquip*>(mItemDataBase)->mIsLocked;
	}
	bool IsItemEquip() const noexcept
	{
		return (ItemType::EQUIP == mItemTemplate->GetType()) ? true : false;
	}


	// getters
public:
	InventoryComponent* GetInventory() noexcept { return mOwnerInventory; }

	StatPairContainer& GetStatPairContainer() noexcept { return mStatPairContainer; }
	const StatPairContainer& GetStatPairContainer() const noexcept { return mStatPairContainer; }

	// ���� from ItemTemplate
	const ItemTemplate* GetItemTemplate() const noexcept { return mItemTemplate; }
	// �ִ� ���� ����
	ItemCount GetMaxStackCount() const { return mItemTemplate->GetMaxStackCount(); }
	// ���� ������ �������ΰ�? (1���� Ŀ�� ���� ������ ��)
	bool IsStackable() const { return mItemTemplate->IsStackable(); }

	ItemType GetType() const { return mItemTemplate->GetType(); }
	ItemCategory GetCategory() const { return mItemTemplate->GetCategory(); }
};