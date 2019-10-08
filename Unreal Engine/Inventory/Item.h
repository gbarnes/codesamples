// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateBrush.h"
#include "Engine/AssetManager.h"
#include "Item.generated.h"

UENUM(BlueprintType)
enum class EQualityType : uint8
{
	QT_Common 	UMETA(DisplayName = "Common"),
	QT_Uncommon UMETA(DisplayName = "Uncommon"),
	QT_Rare 	UMETA(DisplayName = "Rare"),
	QT_Epic 	UMETA(DisplayName = "Epic"),
	QT_Legendary UMETA(DisplayName = "Legendary")
};

/**
 *
 */
UCLASS(BlueprintType)
class  UItem : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|General")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|General")
	EQualityType Quality;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|General")
	FPrimaryAssetType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Inventory")
	bool bIsStackable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Inventory")
	int32 MaxStackSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
	FSlateBrush ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
	TSubclassOf<AActor> Spawnable;

	UFUNCTION()
	FString GetIdentifierString();

	UFUNCTION()
	AActor* Spawn(FVector Location, FQuat Rotation, FVector Force, AActor* Owner);

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};


UCLASS(BlueprintType)
class UItemInstance : public UObject
{
	GENERATED_BODY()
public:
	UItemInstance()
	{
		Item = nullptr;
		Guid = FGuid::NewGuid();
	}

	UItemInstance(UItem* InItemData)
		: Item(InItemData)
	{
		Guid = FGuid::NewGuid();
	}


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Instance")
	UItem* Item;


	bool operator==(const UItemInstance& rhs) const
	{
		return Guid == rhs.Guid;
	}


protected:
	FGuid Guid;
};
