// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Item.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotRefreshed, int32, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotsRefreshed, TArray<int32>, Slots);

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()
public:

	FInventorySlot()
		: Instance(nullptr), Stack(0), SlotIndex(-1)
	{
	}

	FInventorySlot(int32 Index)
		: Instance(nullptr), Stack(0), SlotIndex(Index)
	{
	}

	FInventorySlot(int32 Index, UItemInstance* InInstance)
		: Instance(InInstance), Stack(0), SlotIndex(Index)
	{
	}

	bool FreeCount(int32 InCount)
	{
		if (InCount <= 0)
			return false;

		Stack = FMath::Max(0, Stack - InCount);
		if (Stack == 0)
		{
			Instance = nullptr;
		}

		return true;
	}

	bool Occupy(UItemInstance* InInstance)
	{
		if (!InInstance || !InInstance->Item)
			return false;

		if (IsFull())
			return false;

		this->Instance = InInstance;
		this->Stack += 1;
		return true;
	}

	bool IncreaseStack(int32 InCount)
	{
		if (InCount == 0)
			return false;

		int32 NewStackSize = (GetStackSize() + InCount);

		if (NewStackSize > GetMaxStackSize())
			return false;

		this->Stack = NewStackSize;

		return true;
	}

	UItemInstance* CopyInstance()
	{
		if (!Instance)
			return nullptr;

		auto copy = NewObject<UItemInstance>();
		if (copy) 
		{
			copy->Item = Instance->Item;
		}

		return copy;
	}

	void Free()
	{
		FreeCount(GetStackSize());
	}

	UItemInstance* GetInstance()
	{
		if (IsEmpty())
			return nullptr;

		return Instance;
	}


	int32 GetMaxStackSize() const
	{
		if (Instance == nullptr)
			return 0;

		if (Instance->Item == nullptr)
			return 1;

		// since we cannot be sure that designers (me)
		// always set the correct max stack size value 
		// we default to 1 when the item is not stackable!
		if (!Instance->Item->bIsStackable)
			return 1;

		return Instance->Item->MaxStackSize;
	}

	int32 GetStackSize() const
	{
		return Stack;
	}

	int32 CalcAvailableSpace() const
	{
		return GetMaxStackSize() - GetStackSize();
	}

	bool IsEmpty() const
	{
		return Stack == 0 || Instance == nullptr;
	}

	bool IsFull() const
	{
		if (IsEmpty())
			return false;

		return GetMaxStackSize() == GetStackSize();
	}


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InventorySlot|Item")
	UItemInstance* Instance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InventorySlot|Item")
	int32 Stack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InventorySlot|Item")
	int32 SlotIndex = 0;
};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class  UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventorySlotRefreshed SlotRefreshed;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventorySlotsRefreshed SlotsRefreshed;

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool AddItem(UItem* Item, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool RemoveItem(UItem* Item, int32 Count, int32& RemainingItemCount);

	bool RemoveItem(UItem* Item, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool RemoveInstance(UItemInstance* Instance);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool GetItemsOfType(TArray<UItem*>& List, FPrimaryAssetType Type);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool GetInstancesOfType(TArray<UItemInstance*>& List, FPrimaryAssetType Type);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool GetItems(TArray<UItem*>& List);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool GetInstances(TArray<UItemInstance*>& List);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	UItemInstance* GetItemInstanceAtIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool GetSlots(TArray<FInventorySlot>& List);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool GetSlot(int32 Index, FInventorySlot& Slot);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool IsFull();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool HasItemWithAmount(UItem* Item, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool HasItem(UItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	int32 GetAmountOfItems() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	int32 GetAmountOfItemsWithStackSize() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	int32 GetMaxSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Debug")
	FString ToString();

protected:
	
	UPROPERTY()
	int32 MaxSlots;

	UPROPERTY()
	TArray<FInventorySlot> Slots;

	bool FindSlotsWithItem(UItem* Item, TArray<FInventorySlot>& OutList);
	bool FindFirstFreeSlotForStack(UItem* InItem, FInventorySlot& OutSlot);
	bool FindFirstFreeSlot(FInventorySlot& OutSlot);
	bool FindSlotOfInstance(UItemInstance* Instance, FInventorySlot& OutSlot);

private:
	bool AddNewInstanceToSlot(FInventorySlot& InSlot, UItem* Item, int32 StackSize);
	int32 HandleNonStackableItem(UItem* Item, int32 Count);
	bool FindSlotForStackableItem(UItem* Item, FInventorySlot& OutSlot);
	int32 AddStackedItemToSlot(FInventorySlot& InSlot, UItem* Item, int32 StackAmount);
};

