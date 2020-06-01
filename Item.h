#pragma once

#include "ServerCommon/Datasheet/Template/ItemTemplate.h"
#include "GameObjectComponent/StatComponent/StatPairContainer.h"

class InventoryComponent;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// World 서버가 들고 있어야 할 아이템 정보 (DB로부터 얻어낸 동적 데이터가 주된 정보)
// 아이템과 장비를 분리하는 것이 좋을지 검토가 필요함
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Item : public Disposable
{
private:
	ItemData* mItemDataBase = nullptr;				// 아이템 정보
	InventoryComponent* mOwnerInventory = nullptr;  // 이 아이템이 들어가 있는 인벤토리    
	const ItemTemplate* mItemTemplate = nullptr;    // 아이템 템플릿
	StatPairContainer   mStatPairContainer;         // 아이템에 부여된 스탯을 전부 가진다.


public:
	explicit Item(InventoryComponent* ownerInventory);
	~Item();


public:
	// ItemData가 반드시 정확하게 채워진 채 호출되어야 한다
	// 특히, 장비일 경우 랜덤 스탯은 분명하게!
	bool LoadItem(const ItemDataGateway& itemData);

	// ItemData가 반드시 정확하게 채워진 채 호출되어야 한다
	// 특히, 장비일 경우 랜덤 스탯은 분명하게!
	bool LoadItem(const ItemData& itemData);


	// 스탯 관련
public:
	// 최저 차수의 스탯 오더를 반환. 스탯이 없으면 없으면 STAT_ORDER_MAX를 반환.
	StatOrder GetMinStatOrder() const;

private:
	void CalcStats();


	// StackCount 관련
public:
	// 스택 카운트를 입력하고, 남는 스택 카운트가 유의미할 때 반환 (0보다 같거나 클 때만 유의미)
	ItemCount SetCount(ItemCount count);

	// 스택 카운트를 변경하고, 남는 스택 카운트가 유의미할 때 반환 (0보다 같거나 클 때만 유의미)
	ItemCount ChangeCount(int16 delta);

	// 남은 스택 개수
	ItemCount GetRemainStackCount() const;


	// 착용 아이템 관련
public:
	// 잠금/잠금해제 상태 변경
	bool UpdateLock(bool locked);

	// 레벨, 등급, 경험치 변경
	bool UpdateLevel(Level level, ItemGrade grade, Exp exp);

	// 착용 변경
	bool UpdatePreset(PresetValue presetValue);

	// 소켓팅 변경
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

	// 이하 from ItemTemplate
	const ItemTemplate* GetItemTemplate() const noexcept { return mItemTemplate; }
	// 최대 스택 개수
	ItemCount GetMaxStackCount() const { return mItemTemplate->GetMaxStackCount(); }
	// 스택 가능한 아이템인가? (1보다 커야 스택 가능한 것)
	bool IsStackable() const { return mItemTemplate->IsStackable(); }

	ItemType GetType() const { return mItemTemplate->GetType(); }
	ItemCategory GetCategory() const { return mItemTemplate->GetCategory(); }
};