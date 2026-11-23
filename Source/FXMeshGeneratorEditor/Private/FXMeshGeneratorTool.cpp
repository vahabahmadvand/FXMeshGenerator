// Copyright Epic Games, Inc. All Rights Reserved.

#include "FXMeshGeneratorTool.h"

#include "AssetToolsModule.h"
#include "BaseGizmos/CombinedTransformGizmo.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "BaseGizmos/TransformProxy.h"
#include "Components/StaticMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMeshToMeshDescription.h"
#include "Engine/StaticMesh.h"
#include "FXMeshBuilder.h"
#include "FXMeshDiagnosticMaterials.h"
#include "FXMeshRecipeAsset.h"
#include "IAssetTools.h"
#include "InteractiveGizmoManager.h"
#include "InteractiveToolManager.h"
#include "MeshDescription.h"
#include "ModelingObjectsCreationAPI.h"
#include "Selection/ToolSelectionUtil.h"
#include "StaticMeshAttributes.h"
#include "ToolSetupUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FXMeshGeneratorTool)

using namespace UE::Geometry;

#define LOCTEXT_NAMESPACE "UFXMeshGeneratorTool"

bool UFXMeshGeneratorToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return true;
}

UInteractiveTool* UFXMeshGeneratorToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UFXMeshGeneratorTool* NewTool = NewObject<UFXMeshGeneratorTool>(SceneState.ToolManager);
	NewTool->SetWorld(SceneState.World);
	return NewTool;
}

void UFXMeshGeneratorToolProperties::LoadFromRecipe()
{
	if (ParentTool.IsValid())
	{
		ParentTool->HandleLoadFromRecipe();
	}
}

void UFXMeshGeneratorToolProperties::SaveToRecipe()
{
	if (ParentTool.IsValid())
	{
		ParentTool->HandleSaveToRecipe();
	}
}

void UFXMeshGeneratorToolProperties::LoadFromTargetStaticMesh()
{
	if (ParentTool.IsValid())
	{
		ParentTool->HandleLoadFromTargetStaticMesh();
	}
}

void UFXMeshGeneratorTool::SetWorld(UWorld* World)
{
	TargetWorld = World;
}

void UFXMeshGeneratorTool::Setup()
{
	UInteractiveTool::Setup();

	SetToolDisplayName(LOCTEXT("FXMeshToolDisplayName", "FX Mesh"));

	OutputTypeProperties = NewObject<UCreateMeshObjectTypeProperties>(this);
	OutputTypeProperties->RestoreProperties(this);
	OutputTypeProperties->InitializeDefault();
	OutputTypeProperties->WatchProperty(OutputTypeProperties->OutputType, [this](FString)
	{
		OutputTypeProperties->UpdatePropertyVisibility();
	});
	AddToolPropertySource(OutputTypeProperties);

	FXMeshProperties = NewObject<UFXMeshGeneratorToolProperties>(this);
	FXMeshProperties->ParentTool = this;
	FXMeshProperties->RestoreProperties(this);
	LastAppliedTemplate = FXMeshProperties->Template;
	AddToolPropertySource(FXMeshProperties);

	MaterialProperties = NewObject<UNewMeshMaterialProperties>(this);
	MaterialProperties->RestoreProperties(this);
	AddToolPropertySource(MaterialProperties);

	PreviewMesh = NewObject<UPreviewMesh>(this);
	PreviewMesh->CreateInWorld(TargetWorld, FTransform::Identity);
	ToolSetupUtil::ApplyRenderingConfigurationToPreview(PreviewMesh, nullptr);

	TransformProxy = NewObject<UTransformProxy>(this);
	TransformProxy->SetTransform(PreviewMesh->GetTransform());
	TransformProxy->OnTransformChanged.AddWeakLambda(this, [this](UTransformProxy*, FTransform NewTransform)
	{
		if (PreviewMesh)
		{
			PreviewMesh->SetTransform(NewTransform);
		}
	});

	TransformGizmo = UE::TransformGizmoUtil::CreateCustomTransformGizmo(
		GetToolManager(),
		ETransformGizmoSubElements::StandardTranslateRotate,
		this);
	if (TransformGizmo)
	{
		TransformGizmo->SetActiveTarget(TransformProxy, GetToolManager());
	}

	UpdatePreviewMaterial();
	UpdatePreviewMesh();
}

void UFXMeshGeneratorTool::Shutdown(EToolShutdownType ShutdownType)
{
	if (OutputTypeProperties)
	{
		OutputTypeProperties->SaveProperties(this);
	}
	if (FXMeshProperties)
	{
		FXMeshProperties->SaveProperties(this);
	}
	if (MaterialProperties)
	{
		MaterialProperties->SaveProperties(this);
	}

	if (ShutdownType == EToolShutdownType::Accept)
	{
		GenerateAsset();
	}

	if (PreviewMesh)
	{
		PreviewMesh->Disconnect();
		PreviewMesh = nullptr;
	}

	if (GetToolManager() && GetToolManager()->GetPairedGizmoManager())
	{
		GetToolManager()->GetPairedGizmoManager()->DestroyAllGizmosByOwner(this);
	}
	TransformGizmo = nullptr;
	TransformProxy = nullptr;

	UInteractiveTool::Shutdown(ShutdownType);
}

void UFXMeshGeneratorTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	if (PropertySet == MaterialProperties)
	{
		UpdatePreviewMaterial();
	}
	else if (PropertySet == FXMeshProperties && FXMeshProperties)
	{
		if (FXMeshProperties->Template != LastAppliedTemplate)
		{
			LastAppliedTemplate = FXMeshProperties->Template;
			if (FXMeshProperties->Template != EFXMeshTemplate::Custom)
			{
				FFXMeshBuilder::ApplyTemplate(
					FXMeshProperties->Template,
					FXMeshProperties->Settings,
					FXMeshProperties->Modifiers,
					FXMeshProperties);
			}
		}

		UpdatePreviewMaterial();
		UpdatePreviewMesh();
	}
}

void UFXMeshGeneratorTool::UpdatePreviewMaterial()
{
	if (!PreviewMesh || !MaterialProperties || !FXMeshProperties)
	{
		return;
	}

	UMaterialInterface* ActiveMat = FFXMeshDiagnosticMaterials::GetDiagnosticMaterial(
		FXMeshProperties->DiagnosticView,
		GetToolManager(),
		MaterialProperties->Material.Get());

	PreviewMesh->SetMaterial(ActiveMat);
	PreviewMesh->EnableWireframe(MaterialProperties->bShowWireframe);
}

void UFXMeshGeneratorTool::UpdatePreviewMesh()
{
	if (!PreviewMesh || !FXMeshProperties)
	{
		return;
	}

	FDynamicMesh3 Mesh;
	FFXMeshBuilder::BuildMesh(FXMeshProperties->Settings, FXMeshProperties->Modifiers, Mesh);

	const int32 VertCount = Mesh.VertexCount();
	const int32 TriCount = Mesh.TriangleCount();

	PreviewMesh->UpdatePreview(MoveTemp(Mesh));

	if (GetToolManager())
	{
		const FText ModeHint = FXMeshProperties->TargetStaticMeshToUpdate
			? FText::Format(LOCTEXT("FXMeshRebakeHint", "Re-bake Target: {0} | "), FText::FromString(FXMeshProperties->TargetStaticMeshToUpdate->GetName()))
			: FText::GetEmpty();

		GetToolManager()->DisplayMessage(
			FText::Format(
				LOCTEXT("FXMeshStatsMessage", "{0}Verts: {1}  |  Tris: {2}  |  Click Accept to bake."),
				ModeHint,
				FText::AsNumber(VertCount),
				FText::AsNumber(TriCount)),
			EToolMessageLevel::UserNotification);
	}
}

void UFXMeshGeneratorTool::CloneModifiers(
	const TArray<TObjectPtr<UFXMeshModifier>>& Source,
	TArray<TObjectPtr<UFXMeshModifier>>& Dest,
	UObject* DestOuter)
{
	Dest.Reset();
	if (!DestOuter)
	{
		return;
	}

	for (const TObjectPtr<UFXMeshModifier>& Mod : Source)
	{
		if (Mod)
		{
			Dest.Add(DuplicateObject<UFXMeshModifier>(Mod, DestOuter));
		}
	}
}

void UFXMeshGeneratorTool::HandleLoadFromRecipe()
{
	if (!FXMeshProperties || !FXMeshProperties->RecipePreset)
	{
		if (GetToolManager())
		{
			GetToolManager()->DisplayMessage(
				LOCTEXT("FXMeshNoRecipeLoad", "Assign a Recipe Preset asset first before loading."),
				EToolMessageLevel::UserWarning);
		}
		return;
	}

	FXMeshProperties->Template = EFXMeshTemplate::Custom;
	LastAppliedTemplate = EFXMeshTemplate::Custom;
	FXMeshProperties->Settings = FXMeshProperties->RecipePreset->Settings;
	CloneModifiers(FXMeshProperties->RecipePreset->Modifiers, FXMeshProperties->Modifiers, FXMeshProperties);
	UpdatePreviewMesh();
}

void UFXMeshGeneratorTool::HandleSaveToRecipe()
{
	if (!FXMeshProperties)
	{
		return;
	}

	if (!FXMeshProperties->RecipePreset)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UFXMeshRecipeAssetFactory* Factory = NewObject<UFXMeshRecipeAssetFactory>();
		UObject* CreatedAsset = AssetTools.CreateAssetWithDialog(
			TEXT("DA_FXMeshRecipe"),
			TEXT("/Game"),
			UFXMeshRecipeAsset::StaticClass(),
			Factory);

		FXMeshProperties->RecipePreset = Cast<UFXMeshRecipeAsset>(CreatedAsset);
		if (!FXMeshProperties->RecipePreset)
		{
			return;
		}
	}

	FXMeshProperties->RecipePreset->Modify();
	FXMeshProperties->RecipePreset->Settings = FXMeshProperties->Settings;
	CloneModifiers(FXMeshProperties->Modifiers, FXMeshProperties->RecipePreset->Modifiers, FXMeshProperties->RecipePreset);
	FXMeshProperties->RecipePreset->MarkPackageDirty();

	if (GetToolManager())
	{
		GetToolManager()->DisplayMessage(
			FText::Format(LOCTEXT("FXMeshRecipeSaved", "Saved recipe to {0}."), FText::FromString(FXMeshProperties->RecipePreset->GetName())),
			EToolMessageLevel::UserNotification);
	}
}

void UFXMeshGeneratorTool::HandleLoadFromTargetStaticMesh()
{
	if (!FXMeshProperties || !FXMeshProperties->TargetStaticMeshToUpdate)
	{
		if (GetToolManager())
		{
			GetToolManager()->DisplayMessage(
				LOCTEXT("FXMeshNoTargetMeshLoad", "Assign a Target Static Mesh first before loading its recipe."),
				EToolMessageLevel::UserWarning);
		}
		return;
	}

	UFXMeshRecipeUserData* UserData = FXMeshProperties->TargetStaticMeshToUpdate->GetAssetUserData<UFXMeshRecipeUserData>();
	if (!UserData)
	{
		if (GetToolManager())
		{
			GetToolManager()->DisplayMessage(
				LOCTEXT("FXMeshNoUserData", "Target Static Mesh has no saved FX Mesh recipe metadata."),
				EToolMessageLevel::UserWarning);
		}
		return;
	}

	FXMeshProperties->Template = EFXMeshTemplate::Custom;
	LastAppliedTemplate = EFXMeshTemplate::Custom;
	FXMeshProperties->Settings = UserData->Settings;
	CloneModifiers(UserData->Modifiers, FXMeshProperties->Modifiers, FXMeshProperties);
	UpdatePreviewMesh();
}

void UFXMeshGeneratorTool::SaveRecipeUserDataToStaticMesh(UStaticMesh* TargetMesh) const
{
	if (!TargetMesh || !FXMeshProperties)
	{
		return;
	}

	UFXMeshRecipeUserData* UserData = TargetMesh->GetAssetUserData<UFXMeshRecipeUserData>();
	if (!UserData)
	{
		UserData = NewObject<UFXMeshRecipeUserData>(TargetMesh);
		TargetMesh->AddAssetUserData(UserData);
	}

	UserData->Settings = FXMeshProperties->Settings;
	CloneModifiers(FXMeshProperties->Modifiers, UserData->Modifiers, UserData);
}

bool UFXMeshGeneratorTool::UpdateExistingStaticMesh(UStaticMesh* TargetMesh, const FDynamicMesh3& SourceMesh)
{
	if (!TargetMesh)
	{
		return false;
	}

	TargetMesh->Modify();

	FMeshDescription MeshDescription;
	FStaticMeshAttributes StaticMeshAttributes(MeshDescription);
	StaticMeshAttributes.Register();

	FDynamicMeshToMeshDescription Converter;
	Converter.Convert(&SourceMesh, MeshDescription, true);

	TArray<const FMeshDescription*> MeshDescriptions;
	MeshDescriptions.Add(&MeshDescription);

	UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
	BuildParams.bBuildSimpleCollision = false;
	BuildParams.bCommitMeshDescription = true;
	BuildParams.bMarkPackageDirty = true;

	TargetMesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams);
	SaveRecipeUserDataToStaticMesh(TargetMesh);
	TargetMesh->PostEditChange();
	TargetMesh->MarkPackageDirty();

	return true;
}

void UFXMeshGeneratorTool::GenerateAsset()
{
	if (!PreviewMesh || !OutputTypeProperties || !FXMeshProperties)
	{
		return;
	}

	const FDynamicMesh3* CurMesh = PreviewMesh->GetPreviewDynamicMesh();
	if (!CurMesh)
	{
		return;
	}

	// Reference-Preserving In-Place Update when TargetStaticMeshToUpdate is set
	if (FXMeshProperties->TargetStaticMeshToUpdate)
	{
		GetToolManager()->BeginUndoTransaction(LOCTEXT("FXMeshRebakeTransaction", "Update Existing FX Static Mesh"));
		UpdateExistingStaticMesh(FXMeshProperties->TargetStaticMeshToUpdate, *CurMesh);
		GetToolManager()->EndUndoTransaction();
		return;
	}

	UMaterialInterface* Material = MaterialProperties ? MaterialProperties->Material.Get() : nullptr;

	GetToolManager()->BeginUndoTransaction(LOCTEXT("FXMeshToolTransactionName", "Create FX Mesh"));

	FCreateMeshObjectParams NewMeshObjectParams;
	NewMeshObjectParams.TargetWorld = TargetWorld;
	NewMeshObjectParams.Transform = PreviewMesh->GetTransform();
	NewMeshObjectParams.BaseName = TEXT("FXMesh");
	if (Material)
	{
		NewMeshObjectParams.Materials.Add(Material);
	}
	NewMeshObjectParams.SetMesh(CurMesh);
	OutputTypeProperties->ConfigureCreateMeshObjectParams(NewMeshObjectParams);

	FCreateMeshObjectResult Result = UE::Modeling::CreateMeshObject(GetToolManager(), MoveTemp(NewMeshObjectParams));
	if (Result.IsOK())
	{
		if (UStaticMesh* CreatedSM = Cast<UStaticMesh>(Result.NewAsset))
		{
			SaveRecipeUserDataToStaticMesh(CreatedSM);
			CreatedSM->MarkPackageDirty();
		}
		if (Result.NewActor != nullptr)
		{
			ToolSelectionUtil::SetNewActorSelection(GetToolManager(), Result.NewActor);
		}
	}

	GetToolManager()->EndUndoTransaction();
}

#undef LOCTEXT_NAMESPACE
