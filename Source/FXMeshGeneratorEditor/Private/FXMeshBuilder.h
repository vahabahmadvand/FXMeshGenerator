// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FXMeshGeneratorTypes.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

class FFXMeshBuilder
{
public:
	static void BuildMesh(
		const FFXMeshGeneratorSettings& Settings,
		const TArray<TObjectPtr<UFXMeshModifier>>& Modifiers,
		UE::Geometry::FDynamicMesh3& OutMesh);

	static void ApplyTemplate(
		EFXMeshTemplate Template,
		FFXMeshGeneratorSettings& OutSettings,
		TArray<TObjectPtr<UFXMeshModifier>>& OutModifiers,
		UObject* ModifierOuter);
};
