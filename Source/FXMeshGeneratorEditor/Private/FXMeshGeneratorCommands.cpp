// Copyright Epic Games, Inc. All Rights Reserved.

#include "FXMeshGeneratorCommands.h"

#include "FXMeshGeneratorStyle.h"

#define LOCTEXT_NAMESPACE "FXMeshGeneratorCommands"

FFXMeshGeneratorCommands::FFXMeshGeneratorCommands()
	: TCommands<FFXMeshGeneratorCommands>(
		TEXT("FXMeshGeneratorCommands"),
		NSLOCTEXT("Contexts", "FXMeshGeneratorCommands", "FX Mesh Generator Modeling Tools"),
		NAME_None,
		FFXMeshGeneratorStyle::GetStyleSetName())
{
}

void FFXMeshGeneratorCommands::RegisterCommands()
{
	UI_COMMAND(FXMeshTabButton, "FX Mesh", "FX Mesh Generator Toolset", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(BeginFXMeshTool, "FX Mesh", "Create a customizable procedural VFX mesh (tapered tube, cone, slash arc, or twisted vortex) with UV controls", EUserInterfaceActionType::ToggleButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE
