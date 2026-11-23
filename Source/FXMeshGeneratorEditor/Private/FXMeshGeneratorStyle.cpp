// Copyright Epic Games, Inc. All Rights Reserved.

#include "FXMeshGeneratorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"

TSharedPtr<FSlateStyleSet> FFXMeshGeneratorStyle::StyleSet = nullptr;

TSharedPtr<ISlateStyle> FFXMeshGeneratorStyle::Get()
{
	return StyleSet;
}

FName FFXMeshGeneratorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("FXMeshGeneratorStyle"));
	return StyleSetName;
}

void FFXMeshGeneratorStyle::Initialize()
{
	if (StyleSet.IsValid())
	{
		return;
	}

	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);

	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("FXMeshGenerator")))
	{
		const FString IconsDir = Plugin->GetBaseDir() / TEXT("Resources/Icons");
		const FString TabIconPath = IconsDir / TEXT("FXMeshTab.svg");
		const FString ToolIconPath = IconsDir / TEXT("FXMeshTool.svg");

		StyleSet->Set("FXMeshGeneratorCommands.FXMeshTabButton", new FSlateVectorImageBrush(TabIconPath, Icon40x40));
		StyleSet->Set("FXMeshGeneratorCommands.FXMeshTabButton.Small", new FSlateVectorImageBrush(TabIconPath, Icon20x20));
		StyleSet->Set("FXMeshGeneratorCommands.BeginFXMeshTool", new FSlateVectorImageBrush(ToolIconPath, Icon40x40));
		StyleSet->Set("FXMeshGeneratorCommands.BeginFXMeshTool.Small", new FSlateVectorImageBrush(ToolIconPath, Icon20x20));
	}

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FFXMeshGeneratorStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}
