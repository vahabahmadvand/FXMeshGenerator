// Copyright Epic Games, Inc. All Rights Reserved.

#include "FXMeshGeneratorEditorModule.h"

#include "FXMeshGeneratorCommands.h"
#include "FXMeshGeneratorStyle.h"
#include "FXMeshGeneratorTool.h"

#include "Features/IModularFeatures.h"

#define LOCTEXT_NAMESPACE "FFXMeshGeneratorEditorModule"

void FFXMeshGeneratorEditorModule::StartupModule()
{
	FFXMeshGeneratorStyle::Initialize();
	FFXMeshGeneratorCommands::Register();

	IModularFeatures::Get().RegisterModularFeature(IModelingModeToolExtension::GetModularFeatureName(), this);
}

void FFXMeshGeneratorEditorModule::ShutdownModule()
{
	IModularFeatures::Get().UnregisterModularFeature(IModelingModeToolExtension::GetModularFeatureName(), this);

	FFXMeshGeneratorCommands::Unregister();
	FFXMeshGeneratorStyle::Shutdown();
}

FText FFXMeshGeneratorEditorModule::GetExtensionName()
{
	return LOCTEXT("FXMeshGenerator_ExtensionName", "FXMeshGenerator");
}

FText FFXMeshGeneratorEditorModule::GetToolSectionName()
{
	return LOCTEXT("FXMeshGenerator_SectionName", "FX Mesh");
}

void FFXMeshGeneratorEditorModule::GetExtensionTools(const FExtensionToolQueryInfo& QueryInfo, TArray<FExtensionToolDescription>& ToolsOut)
{
	const FFXMeshGeneratorCommands& Commands = FFXMeshGeneratorCommands::Get();

	FExtensionToolDescription ToolDesc;
	ToolDesc.ToolName = LOCTEXT("FXMeshGenerator_ToolName", "FX Mesh");
	ToolDesc.ToolCommand = Commands.BeginFXMeshTool;
	if (!QueryInfo.bIsInfoQueryOnly)
	{
		ToolDesc.ToolBuilder = NewObject<UFXMeshGeneratorToolBuilder>();
	}
	ToolsOut.Add(ToolDesc);
}

bool FFXMeshGeneratorEditorModule::GetExtensionExtendedInfo(FModelingModeExtensionExtendedInfo& InfoOut)
{
	InfoOut.ToolPaletteButtonTooltip = LOCTEXT("FXMeshGenerator_PaletteTooltip", "Procedural VFX Mesh Generation Tools");
	InfoOut.ExtensionCommand = FFXMeshGeneratorCommands::Get().FXMeshTabButton;
	return true;
}

IMPLEMENT_MODULE(FFXMeshGeneratorEditorModule, FXMeshGeneratorEditor)

#undef LOCTEXT_NAMESPACE
