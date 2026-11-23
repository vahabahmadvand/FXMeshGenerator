// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FXMeshGeneratorTypes.h"

class UInteractiveToolManager;
class UMaterialInterface;

class FFXMeshDiagnosticMaterials
{
public:
	static UMaterialInterface* GetDiagnosticMaterial(
		EFXMeshDiagnosticView ViewMode,
		UInteractiveToolManager* ToolManager,
		UMaterialInterface* DefaultMaterial);
};
