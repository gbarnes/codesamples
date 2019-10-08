// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Engine/World.h"

FPrimaryAssetId UItem::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(ItemType, GetFName());
}

FString UItem::GetIdentifierString()
{
	return GetPrimaryAssetId().ToString();
}

AActor* UItem::Spawn(FVector Location, FQuat Rotation, FVector Force, AActor* Owner)
{
	FActorSpawnParameters Params;
	Params.Owner = Owner;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.OverrideLevel = Owner->GetLevel();

	auto Actor = Owner->GetWorld()->SpawnActor<AActor>(this->Spawnable.Get(), Location, Rotation.Rotator(), Params);
	return Actor;
}