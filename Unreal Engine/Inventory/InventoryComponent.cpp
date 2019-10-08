// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"


// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 4;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	for (int i = 0; i < MaxSlots; i++)
		this->Slots.Add(FInventorySlot(i));
}

bool UInventoryComponent::AddItem(UItem* Item, int32 Count)
{
	bool bChanged = false;

	if (!Item) 
	{
		UE_LOG(LogTemp, Fatal, TEXT("Passed item is null!"));
		return false;
	}

	if (IsFull())
	{
		UE_LOG(LogTemp, Warning, TEXT("All slots are fully occupied!"));
		return false;
	}

	if (!Item->bIsStackable)
	{
		HandleNonStackableItem(Item, Count);
		return true;
	}
	else 
	{
		FInventorySlot Slot;
		bool bHasSlot = FindSlotForStackableItem(Item, Slot);
		if (bHasSlot)
		{
			TArray<int32> ChangedSlots;
			int32 slotAmount = FMath::DivideAndRoundUp(Count, Item->MaxStackSize);
			int32 overshootAmount = AddStackedItemToSlot(Slot, Item, Count);

			// Note (gb) when this happens there was some problem 
			// because effectively we didn't add any item at all!
			if (overshootAmount == Count) 
			{
				UE_LOG(LogTemp, Warning, TEXT("Eventhough we found a valid slot we somehow couldn't add any stack amount at all to the inventory."));
				return false;
			}

			ChangedSlots.Add(Slot.SlotIndex);

			if (overshootAmount > 0)
			{
				bHasSlot = FindFirstFreeSlot(Slot);
				while (overshootAmount > 0 && bHasSlot)
				{
					overshootAmount = AddStackedItemToSlot(Slot, Item, overshootAmount);
					ChangedSlots.Add(Slot.SlotIndex);
					bHasSlot = FindFirstFreeSlot(Slot);
				}
			}

			SlotsRefreshed.Broadcast(ChangedSlots);
			return true;
		}
	}

	return bChanged;
}

bool UInventoryComponent::RemoveItem(UItem* Item, int32 Count)
{
	int32 RemovedItems = 0;
	return RemoveItem(Item, Count, RemovedItems);
}

bool UInventoryComponent::RemoveItem(UItem* Item, int32 Count, int32& RemovedItems)
{
	TArray<FInventorySlot> FoundSlots;
	if (!FindSlotsWithItem(Item, FoundSlots))
		return false;

	TArray<int32> ChangedSlots;

	FoundSlots.Sort([](const FInventorySlot& LHS, const FInventorySlot& RHS)
	{
		return LHS.GetStackSize() < RHS.GetStackSize();
	});
	
	int32 i = 0;
	int32 Index = FoundSlots[i].SlotIndex;
	int32 LeftCount = 0;

	while (Count > 0)
	{
		LeftCount = Count - FoundSlots[i].Stack;
		this->Slots[Index].FreeCount(Count);
		ChangedSlots.Add(Index);

		RemovedItems += Count - LeftCount;
		Count = LeftCount;
		i++;

		// we need to break out of the loop when there aren't enough slots left
		if (i > FoundSlots.Num() - 1)
			break;
			
		Index = FoundSlots[i].SlotIndex;
	}

	SlotsRefreshed.Broadcast(ChangedSlots);
	return (LeftCount > 0) ? false : true;
}

bool UInventoryComponent::RemoveInstance(UItemInstance* Instance)
{
	FInventorySlot Slot;

	if (!FindSlotOfInstance(Instance, Slot))
		return false;

	this->Slots[Slot.SlotIndex].Free();

	TArray<int32> ChangedSlots;
	ChangedSlots.Add(Slot.SlotIndex);
	SlotsRefreshed.Broadcast(ChangedSlots);

	return true;
}

bool UInventoryComponent::GetItemsOfType(TArray<UItem*>& List, FPrimaryAssetType Type)
{
	for (auto& slot : Slots)
	{
		if (!slot.IsEmpty())
		{
			FPrimaryAssetId AssetId = slot.GetInstance()->GetPrimaryAssetId();

			// Filters based on item type
			if (AssetId.PrimaryAssetType == Type || !Type.IsValid())
			{
				List.Add(slot.GetInstance()->Item);
			}
		}
	}

	return true;
}

bool UInventoryComponent::GetInstancesOfType(TArray<UItemInstance*>& List, FPrimaryAssetType Type)
{
	for (auto& slot : Slots)
	{
		if (!slot.IsEmpty())
		{
			FPrimaryAssetId AssetId = slot.GetInstance()->GetPrimaryAssetId();

			// Filters based on item type
			if (AssetId.PrimaryAssetType == Type || !Type.IsValid())
			{
				List.Add(slot.GetInstance());
			}
		}
	}

	return true;
}

bool UInventoryComponent::GetItems(TArray<UItem*>& List)
{
	for (auto& slot : Slots)
	{
		if (slot.IsEmpty())
			continue;

		List.Add(slot.GetInstance()->Item);
	}

	return true;
}

bool UInventoryComponent::GetInstances(TArray<UItemInstance*>& List)
{
	for (auto& slot : Slots)
	{
		if (slot.IsEmpty())
			continue;

		List.Add(slot.GetInstance());
	}

	return true;
}

UItemInstance* UInventoryComponent::GetItemInstanceAtIndex(int32 Index)
{
	if (Index >= this->MaxSlots)
		return nullptr;

	if (this->Slots.Num() == 0)
		return nullptr;

	auto& slot = this->Slots[Index];
	if (slot.IsEmpty())
		return nullptr;

	return slot.Instance;
}

bool UInventoryComponent::GetSlots(TArray<FInventorySlot>& List)
{
	for (auto& slot : Slots)
	{
		List.Add(slot);
	}

	return true;
}

bool UInventoryComponent::GetSlot(int32 Index, FInventorySlot& Slot)
{
	if (Index >= this->MaxSlots)
		return false;

	if (Slots.Num() == 0)
		return false;

	Slot = Slots[Index];

	return true;
}

bool UInventoryComponent::IsFull()
{
	for (FInventorySlot& slot : Slots)
	{
		if (!slot.IsFull())
			return false;
	}

	return true;
}

bool UInventoryComponent::HasItemWithAmount(UItem* Item, int32 Amount)
{
	int32 count = 0;

	auto foundSlots = Slots.FilterByPredicate([Item, &count](const FInventorySlot& InSlot)
	{
		if (InSlot.IsEmpty())
			return false;

		if (InSlot.Instance->Item == Item)
		{
			count += InSlot.GetStackSize();
			return true;
		}

		return false;
	});

	return count >= Amount;
}

bool UInventoryComponent::HasItem(UItem* Item)
{
	auto slot = Slots.FindByPredicate([&](FInventorySlot& InSlot)
	{
		return InSlot.Instance->Item == Item;
	});

	return slot;
}


bool UInventoryComponent::FindFirstFreeSlot(FInventorySlot& OutSlot)
{
	auto slot = Slots.FindByPredicate([&](FInventorySlot& InSlot)
	{
		return InSlot.IsEmpty();
	});

	if (slot)
	{
		OutSlot = *slot;
		return true;
	}

	return false;
}

bool UInventoryComponent::FindSlotOfInstance(UItemInstance* Instance, FInventorySlot& OutSlot)
{
	auto slot = Slots.FindByPredicate([&](FInventorySlot& InSlot)
	{
		return InSlot.GetInstance() == Instance;
	});

	if (slot)
	{
		OutSlot = *slot;
		return true;
	}

	return false;
}

bool UInventoryComponent::FindFirstFreeSlotForStack(UItem* InItem, FInventorySlot& OutSlot)
{
	for (auto& slot : Slots)
	{
		if (slot.IsEmpty() || slot.IsFull())
			continue;

		auto instance = slot.GetInstance();
		if (instance == nullptr)
			continue;

		if (instance->Item != InItem)
			continue;

		OutSlot = slot;
		return true;
	}

	return false;
}

bool UInventoryComponent::FindSlotsWithItem(UItem* Item, TArray<FInventorySlot>& OutList)
{
	bool bFoundSlots = false;

	for (auto& slot : Slots)
	{
		if (slot.IsEmpty())
			continue;

		if (slot.GetInstance()->Item == Item)
		{
			bFoundSlots = true;
			OutList.Add(slot);
		}
	}

	return bFoundSlots;
}

FString UInventoryComponent::ToString()
{
	FString Content;
	Content.Append(FString::Printf(TEXT("There is a total of %i items in the inventory\n"), GetAmountOfItems()));

	for (auto slot : Slots)
	{
		if(slot.IsEmpty())
			Content.Append(FString::Printf(TEXT("{SlotIndex: %i, Is Empty}\n"), slot.SlotIndex));
		else 
			Content.Append(FString::Printf(TEXT("{SlotIndex: %i, ID: %s, Count: %i}\n"), slot.SlotIndex, *slot.Instance->Item->GetIdentifierString(), slot.GetStackSize()));
	}

	return Content;
}

int32 UInventoryComponent::GetAmountOfItems() const
{
	int32 ItemAmount = 0;
	for (auto& slot : Slots)
	{
		if (slot.IsEmpty())
			continue;

		ItemAmount += 1;
	}

	return ItemAmount;
}

int32 UInventoryComponent::GetAmountOfItemsWithStackSize() const
{
	int32 ItemAmount = 0;
	for (auto& slot : Slots)
	{
		ItemAmount += slot.GetStackSize();
	}

	return ItemAmount;
}

int32 UInventoryComponent::GetMaxSlots() const
{
	return MaxSlots;
}


bool UInventoryComponent::AddNewInstanceToSlot(FInventorySlot& InSlot, UItem* Item, int32 StackAmount)
{
	if (Item->MaxStackSize < StackAmount)
		return false;

	if (InSlot.Instance == nullptr)
	{
		auto instance = NewObject<UItemInstance>();
		if (instance)
		{
			instance->Item = Item;
			InSlot.Occupy(instance);
			InSlot.IncreaseStack(StackAmount - 1);

			this->Slots[InSlot.SlotIndex] = InSlot;
			return true;
		}

		return false;
	}

	return false;
}

int32 UInventoryComponent::AddStackedItemToSlot(FInventorySlot& InSlot, UItem* Item, int32 StackAmount)
{
	bool bIsEmpty = InSlot.IsEmpty();
	int32 availableSpace = (bIsEmpty) ? Item->MaxStackSize : InSlot.CalcAvailableSpace();
	int32 overshoot = availableSpace - StackAmount;
	int32 absOvershoot = (overshoot < 0) ? FMath::Abs(overshoot) : 0;
	int32 amountToAdd = (overshoot < 0) ? StackAmount - absOvershoot : StackAmount;

	if (bIsEmpty)
	{
		if (!AddNewInstanceToSlot(InSlot, Item, amountToAdd))
			return StackAmount;

		return absOvershoot;
	}

	InSlot.IncreaseStack(amountToAdd);
	this->Slots[InSlot.SlotIndex] = InSlot;

	return absOvershoot;
}

int32 UInventoryComponent::HandleNonStackableItem(UItem* Item, int32 Count)
{
	TArray<int32> changedSlots;

	// leftover items are items that couldn't be added to the 
	// inventory anymore due to not finding a free slot.
	int leftovers = 0;
	for (int i = 0; i < Count; i++)
	{
		FInventorySlot outSlot;
		auto bhasSlot = FindFirstFreeSlot(outSlot);
		if (!bhasSlot) 
		{
			leftovers = Count - i;
			break;
		}

		if (AddNewInstanceToSlot(outSlot, Item, 1))
			changedSlots.Add(outSlot.SlotIndex);
	}

	SlotsRefreshed.Broadcast(changedSlots);

	return leftovers;
}


bool UInventoryComponent::FindSlotForStackableItem(UItem* Item, FInventorySlot& OutSlot)
{
	auto bHasSlot = FindFirstFreeSlotForStack(Item, OutSlot);
	if (bHasSlot)
	{
		return true;
	}

	return FindFirstFreeSlot(OutSlot);
}