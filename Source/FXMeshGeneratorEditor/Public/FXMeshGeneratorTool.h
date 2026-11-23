// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FXMeshGeneratorTypes.h"
#include "InteractiveTool.h"
#include "InteractiveToolBuilder.h"
#include "PreviewMesh.h"
#include "Properties/MeshMaterialProperties.h"
#include "PropertySets/CreateMeshObjectTypeProperties.h"

#include "FXMeshGeneratorTool.generated.h"

class UCombinedTransformGizmo;
class UFXMeshGeneratorTool;
class UFXMeshRecipeAsset;
class UStaticMesh;
class UTransformProxy;

/**
 * Tool builder for the FX Mesh Generator Modeling Mode tool.
 */
UCLASS()
class FXMESHGENERATOREDITOR_API UFXMeshGeneratorToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

/**
 * Editable properties for generating procedural VFX meshes in Modeling Mode.
 */
UCLASS()
class FXMESHGENERATOREDITOR_API UFXMeshGeneratorToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	/** One-click VFX starting template (populates Shape, Modifiers, and Vertex Color defaults) */
	UPROPERTY(EditAnywhere, Category = "Template")
	EFXMeshTemplate Template = EFXMeshTemplate::Custom;

	/** Core procedural shape, UV, vertex-color, and positioning settings */
	UPROPERTY(EditAnywhere, Category = "FX Mesh", meta = (ShowOnlyInnerProperties))
	FFXMeshGeneratorSettings Settings;

	/** Ordered deformation modifier stack evaluated top-to-bottom */
	UPROPERTY(EditAnywhere, Instanced, Category = "Modifiers")
	TArray<TObjectPtr<UFXMeshModifier>> Modifiers;

	/** Live viewport inspection mode (does not alter the final baked material) */
	UPROPERTY(EditAnywhere, Category = "Diagnostics")
	EFXMeshDiagnosticView DiagnosticView = EFXMeshDiagnosticView::DefaultMaterial;

	/** Optional Recipe DataAsset for loading or saving reusable FX Mesh configurations */
	UPROPERTY(EditAnywhere, Category = "Presets & Re-Bake")
	TObjectPtr<UFXMeshRecipeAsset> RecipePreset = nullptr;

	/** Optional existing StaticMesh to overwrite in-place on Accept (preserves all Niagara/Blueprint references) */
	UPROPERTY(EditAnywhere, Category = "Presets & Re-Bake")
	TObjectPtr<UStaticMesh> TargetStaticMeshToUpdate = nullptr;

	/** Load settings and modifier stack from RecipePreset */
	UFUNCTION(CallInEditor, Category = "Presets & Re-Bake")
	void LoadFromRecipe();

	/** Save current settings and modifier stack into RecipePreset */
	UFUNCTION(CallInEditor, Category = "Presets & Re-Bake")
	void SaveToRecipe();

	/** Load previously baked recipe metadata from TargetStaticMeshToUpdate */
	UFUNCTION(CallInEditor, Category = "Presets & Re-Bake")
	void LoadFromTargetStaticMesh();

	TWeakObjectPtr<UFXMeshGeneratorTool> ParentTool;
};

/**
 * Interactive Modeling Mode tool that previews, inspects, and bakes procedural VFX meshes.
 */
UCLASS()
class FXMESHGENERATOREDITOR_API UFXMeshGeneratorTool : public UInteractiveTool
{
	GENERATED_BODY()

public:
	void SetWorld(UWorld* World);

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual bool HasCancel() const override { return true; }
	virtual bool HasAccept() const override { return true; }
	virtual bool CanAccept() const override { return PreviewMesh != nullptr; }

	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;

	void HandleLoadFromRecipe();
	void HandleSaveToRecipe();
	void HandleLoadFromTargetStaticMesh();

protected:
	void UpdatePreviewMesh();
	void UpdatePreviewMaterial();
	void GenerateAsset();
	bool UpdateExistingStaticMesh(UStaticMesh* TargetMesh, const UE::Geometry::FDynamicMesh3& SourceMesh);
	void SaveRecipeUserDataToStaticMesh(UStaticMesh* TargetMesh) const;
	static void CloneModifiers(const TArray<TObjectPtr<UFXMeshModifier>>& Source, TArray<TObjectPtr<UFXMeshModifier>>& Dest, UObject* DestOuter);

	UPROPERTY()
	TObjectPtr<UCreateMeshObjectTypeProperties> OutputTypeProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UFXMeshGeneratorToolProperties> FXMeshProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UNewMeshMaterialProperties> MaterialProperties = nullptr;

	UPROPERTY()
	TObjectPtr<UPreviewMesh> PreviewMesh = nullptr;

	UPROPERTY()
	TObjectPtr<UTransformProxy> TransformProxy = nullptr;

	UPROPERTY()
	TObjectPtr<UCombinedTransformGizmo> TransformGizmo = nullptr;

	UPROPERTY()
	TObjectPtr<UWorld> TargetWorld = nullptr;

	EFXMeshTemplate LastAppliedTemplate = EFXMeshTemplate::Custom;
};
