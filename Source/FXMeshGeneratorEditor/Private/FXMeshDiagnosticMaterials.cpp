// Copyright Epic Games, Inc. All Rights Reserved.

#include "FXMeshDiagnosticMaterials.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionTwoSidedSign.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "ToolSetupUtil.h"

namespace
{
	UMaterial* GetOrCreateBackfaceMaterial()
	{
		static TWeakObjectPtr<UMaterial> CachedMat;
		if (CachedMat.IsValid())
		{
			return CachedMat.Get();
		}

		UMaterial* Mat = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
		Mat->SetShadingModel(MSM_Unlit);
		Mat->TwoSided = true;

		UMaterialExpressionTwoSidedSign* SignExpr = NewObject<UMaterialExpressionTwoSidedSign>(Mat);
		UMaterialExpressionSaturate* SatExpr = NewObject<UMaterialExpressionSaturate>(Mat);
		SatExpr->Input.Connect(0, SignExpr);

		UMaterialExpressionConstant3Vector* BackColor = NewObject<UMaterialExpressionConstant3Vector>(Mat);
		BackColor->Constant = FLinearColor(0.95f, 0.1f, 0.1f); // Red backfaces

		UMaterialExpressionConstant3Vector* FrontColor = NewObject<UMaterialExpressionConstant3Vector>(Mat);
		FrontColor->Constant = FLinearColor(0.35f, 0.38f, 0.42f); // Neutral front faces

		UMaterialExpressionLinearInterpolate* LerpExpr = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
		LerpExpr->A.Connect(0, BackColor);
		LerpExpr->B.Connect(0, FrontColor);
		LerpExpr->Alpha.Connect(0, SatExpr);

		Mat->GetExpressionCollection().AddExpression(SignExpr);
		Mat->GetExpressionCollection().AddExpression(SatExpr);
		Mat->GetExpressionCollection().AddExpression(BackColor);
		Mat->GetExpressionCollection().AddExpression(FrontColor);
		Mat->GetExpressionCollection().AddExpression(LerpExpr);

		Mat->GetEditorOnlyData()->EmissiveColor.Connect(0, LerpExpr);
		Mat->PreEditChange(nullptr);
		Mat->PostEditChange();

		CachedMat = Mat;
		return Mat;
	}

	UMaterial* GetOrCreateVertexColorChannelMaterial(int32 OutputIndex)
	{
		static TWeakObjectPtr<UMaterial> CachedChannels[4];
		const int32 Slot = FMath::Clamp(OutputIndex - 1, 0, 3);
		if (CachedChannels[Slot].IsValid())
		{
			return CachedChannels[Slot].Get();
		}

		UMaterial* Mat = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
		Mat->SetShadingModel(MSM_Unlit);
		Mat->TwoSided = true;

		UMaterialExpressionVertexColor* VCExpr = NewObject<UMaterialExpressionVertexColor>(Mat);
		Mat->GetExpressionCollection().AddExpression(VCExpr);

		// Output 0 = RGB, 1 = R, 2 = G, 3 = B, 4 = A
		Mat->GetEditorOnlyData()->EmissiveColor.Connect(OutputIndex, VCExpr);
		Mat->PreEditChange(nullptr);
		Mat->PostEditChange();

		CachedChannels[Slot] = Mat;
		return Mat;
	}
}

UMaterialInterface* FFXMeshDiagnosticMaterials::GetDiagnosticMaterial(
	EFXMeshDiagnosticView ViewMode,
	UInteractiveToolManager* ToolManager,
	UMaterialInterface* DefaultMaterial)
{
	switch (ViewMode)
	{
	case EFXMeshDiagnosticView::DefaultMaterial:
		return DefaultMaterial;

	case EFXMeshDiagnosticView::UVChecker:
		return ToolSetupUtil::GetUVCheckerboardMaterial(16.0);

	case EFXMeshDiagnosticView::VertexColorsRGBA:
		return ToolSetupUtil::GetVertexColorMaterial(ToolManager, true);

	case EFXMeshDiagnosticView::VertexColorR:
		return GetOrCreateVertexColorChannelMaterial(1);

	case EFXMeshDiagnosticView::VertexColorG:
		return GetOrCreateVertexColorChannelMaterial(2);

	case EFXMeshDiagnosticView::VertexColorB:
		return GetOrCreateVertexColorChannelMaterial(3);

	case EFXMeshDiagnosticView::VertexColorA:
		return GetOrCreateVertexColorChannelMaterial(4);

	case EFXMeshDiagnosticView::Normals:
		return ToolSetupUtil::GetImageBasedSculptMaterial(ToolManager, ToolSetupUtil::ImageMaterialType::TangentNormalFromView);

	case EFXMeshDiagnosticView::BackfaceHighlight:
		return GetOrCreateBackfaceMaterial();

	default:
		return DefaultMaterial;
	}
}
