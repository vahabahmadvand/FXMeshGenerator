// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "Engine/DataAsset.h"
#include "Factories/Factory.h"
#include "FXMeshGeneratorTypes.h"

#include "FXMeshRecipeAsset.generated.h"

/**
 * Reusable DataAsset storing a complete FX Mesh Generator recipe (shape, UVs, vertex colors, and modifier stack).
 */
UCLASS(BlueprintType)
class FXMESHGENERATOREDITOR_API UFXMeshRecipeAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX Mesh Recipe")
	FFXMeshGeneratorSettings Settings;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "FX Mesh Recipe")
	TArray<TObjectPtr<UFXMeshModifier>> Modifiers;
};

/**
 * AssetUserData attached to baked StaticMeshes so they can be re-loaded into FX Mesh Generator for reference-preserving updates.
 */
UCLASS()
class FXMESHGENERATOREDITOR_API UFXMeshRecipeUserData : public UAssetUserData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "FX Mesh Recipe")
	FFXMeshGeneratorSettings Settings;

	UPROPERTY(EditAnywhere, Instanced, Category = "FX Mesh Recipe")
	TArray<TObjectPtr<UFXMeshModifier>> Modifiers;
};

/**
 * Factory enabling "Create New Asset -> FX Mesh Recipe" in both the Content Browser and asset picker dropdowns.
 */
UCLASS()
class FXMESHGENERATOREDITOR_API UFXMeshRecipeAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UFXMeshRecipeAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual FText GetDisplayName() const override;
};
