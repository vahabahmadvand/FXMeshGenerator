// Copyright Epic Games, Inc. All Rights Reserved.

#include "FXMeshRecipeAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FXMeshRecipeAsset)

#define LOCTEXT_NAMESPACE "FXMeshRecipeAssetFactory"

UFXMeshRecipeAssetFactory::UFXMeshRecipeAssetFactory()
{
	SupportedClass = UFXMeshRecipeAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = false;
}

UObject* UFXMeshRecipeAssetFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UFXMeshRecipeAsset>(InParent, InClass, InName, Flags);
}

FText UFXMeshRecipeAssetFactory::GetDisplayName() const
{
	return LOCTEXT("FXMeshRecipeAssetDisplayName", "FX Mesh Recipe");
}

#undef LOCTEXT_NAMESPACE
