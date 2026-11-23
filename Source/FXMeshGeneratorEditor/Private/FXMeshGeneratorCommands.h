// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FFXMeshGeneratorCommands : public TCommands<FFXMeshGeneratorCommands>
{
public:
	FFXMeshGeneratorCommands();

	TSharedPtr<FUICommandInfo> FXMeshTabButton;
	TSharedPtr<FUICommandInfo> BeginFXMeshTool;

	virtual void RegisterCommands() override;
};
