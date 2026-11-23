// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Math/Box.h"

#include "FXMeshGeneratorTypes.generated.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

UENUM(BlueprintType)
enum class EFXMeshShapeType : uint8
{
	CylinderTube	UMETA(DisplayName = "Cylinder / Tube"),
	Cone			UMETA(DisplayName = "Cone / Funnel"),
	SlashArc		UMETA(DisplayName = "Slash / Swept Arc"),
	DiscRing		UMETA(DisplayName = "Disc / Shockwave Ring"),
	HemisphereDome	UMETA(DisplayName = "Sphere / Hemisphere Dome"),
	Ribbon			UMETA(DisplayName = "Ribbon / Trail Strip"),
	CrossPlane		UMETA(DisplayName = "Cross-Plane / Star Card"),
	Torus			UMETA(DisplayName = "Torus / Portal Ring")
};

UENUM(BlueprintType)
enum class EFXMeshTemplate : uint8
{
	Custom			UMETA(DisplayName = "Custom"),
	SwordSlash		UMETA(DisplayName = "Sword Slash"),
	GroundShockwave	UMETA(DisplayName = "Ground Shockwave"),
	EnergyBeam		UMETA(DisplayName = "Energy Beam"),
	TornadoVortex	UMETA(DisplayName = "Tornado / Vortex"),
	ShieldDome		UMETA(DisplayName = "Shield Dome"),
	FlameCard		UMETA(DisplayName = "Flame Cross-Card"),
	PortalRing		UMETA(DisplayName = "Portal Ring")
};

UENUM(BlueprintType)
enum class EFXMeshUVProjection : uint8
{
	ShapeDefault	UMETA(DisplayName = "Shape Default (Grid/Strip)"),
	PolarRadial		UMETA(DisplayName = "Polar / Radial")
};

UENUM(BlueprintType)
enum class EFXMeshVertexColorSource : uint8
{
	Constant		UMETA(DisplayName = "Constant"),
	AxisGradientU	UMETA(DisplayName = "Gradient Along U (0 -> 1)"),
	AxisGradientV	UMETA(DisplayName = "Gradient Along V (0 -> 1)"),
	EdgeFalloffU	UMETA(DisplayName = "Edge Falloff U (0 -> 1 -> 0)"),
	EdgeFalloffV	UMETA(DisplayName = "Edge Falloff V (0 -> 1 -> 0)"),
	RadialDistance	UMETA(DisplayName = "Radial Distance (Center -> Edge)")
};

UENUM(BlueprintType)
enum class EFXMeshDiagnosticView : uint8
{
	DefaultMaterial		UMETA(DisplayName = "Shaded / Target Material"),
	UVChecker			UMETA(DisplayName = "UV Checker"),
	VertexColorsRGBA	UMETA(DisplayName = "Vertex Colors (RGBA)"),
	VertexColorR		UMETA(DisplayName = "Vertex Color (R)"),
	VertexColorG		UMETA(DisplayName = "Vertex Color (G)"),
	VertexColorB		UMETA(DisplayName = "Vertex Color (B)"),
	VertexColorA		UMETA(DisplayName = "Vertex Color (A)"),
	Normals				UMETA(DisplayName = "Normals"),
	BackfaceHighlight	UMETA(DisplayName = "Backface Highlight (Red)")
};

UENUM(BlueprintType)
enum class EFXMeshPivotPreset : uint8
{
	Base	UMETA(DisplayName = "Base"),
	Center	UMETA(DisplayName = "Center"),
	Top		UMETA(DisplayName = "Top")
};

UENUM(BlueprintType)
enum class EFXMeshAxis : uint8
{
	X UMETA(DisplayName = "X"),
	Y UMETA(DisplayName = "Y"),
	Z UMETA(DisplayName = "Z")
};

// ============================================================================
// Per-Shape Parameter Structs
// ============================================================================

USTRUCT(BlueprintType)
struct FXMESHGENERATOREDITOR_API FFXMeshCylinderParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder", meta = (UIMin = "0.1", UIMax = "500.0", ClampMin = "0.01", ClampMax = "10000.0"))
	float BottomRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder", meta = (UIMin = "0.1", UIMax = "500.0", ClampMin = "0.01", ClampMax = "10000.0"))
	float TopRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder", meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.1", ClampMax = "10000.0"))
	float Height = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder", meta = (UIMin = "15.0", UIMax = "360.0", ClampMin = "5.0", ClampMax = "360.0"))
	float ArcAngle = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder", meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "256"))
	int32 RadialSegments = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder", meta = (UIMin = "1", UIMax = "64", ClampMin = "1", ClampMax = "128"))
	int32 HeightSegments = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	bool bCapBottom = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	bool bCapTop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder")
	bool bUseRadiusCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cylinder", meta = (EditCondition = "bUseRadiusCurve", EditConditionHides))
	FRuntimeFloatCurve RadiusProfileCurve;
};

USTRUCT(BlueprintType)
struct FXMESHGENERATOREDITOR_API FFXMeshConeParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone", meta = (UIMin = "0.1", UIMax = "500.0", ClampMin = "0.01", ClampMax = "10000.0"))
	float BaseRadius = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone", meta = (UIMin = "0.0", UIMax = "500.0", ClampMin = "0.0", ClampMax = "10000.0"))
	float TipRadius = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone", meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.1", ClampMax = "10000.0"))
	float Height = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone", meta = (UIMin = "15.0", UIMax = "360.0", ClampMin = "5.0", ClampMax = "360.0"))
	float ArcAngle = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone", meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "256"))
	int32 RadialSegments = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone", meta = (UIMin = "1", UIMax = "64", ClampMin = "1", ClampMax = "128"))
	int32 HeightSegments = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone")
	bool bCapBase = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone")
	bool bCapTip = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone")
	bool bUseRadiusCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cone", meta = (EditCondition = "bUseRadiusCurve", EditConditionHides))
	FRuntimeFloatCurve RadiusProfileCurve;
};

USTRUCT(BlueprintType)
struct FXMESHGENERATOREDITOR_API FFXMeshSlashArcParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Arc", meta = (UIMin = "1.0", UIMax = "500.0", ClampMin = "1.0", ClampMax = "10000.0"))
	float OuterRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Arc", meta = (UIMin = "0.0", UIMax = "500.0", ClampMin = "0.0", ClampMax = "10000.0"))
	float InnerRadius = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Arc", meta = (UIMin = "15.0", UIMax = "360.0", ClampMin = "5.0", ClampMax = "360.0"))
	float ArcAngle = 160.0f;

	/** Vertical arch height of the slash shell */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Arc", meta = (UIMin = "-200.0", UIMax = "200.0", ClampMin = "-2000.0", ClampMax = "2000.0"))
	float ArchHeight = 12.0f;

	/** Smoothly pinch the inner-to-outer width to zero at the start and end tips */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Arc")
	bool bTaperTips = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Arc", meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "256"))
	int32 ArcSegments = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Arc", meta = (UIMin = "1", UIMax = "32", ClampMin = "1", ClampMax = "64"))
	int32 CrossSegments = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Arc")
	bool bUseWidthCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Arc", meta = (EditCondition = "bUseWidthCurve", EditConditionHides))
	FRuntimeFloatCurve WidthProfileCurve;
};

USTRUCT(BlueprintType)
struct FXMESHGENERATOREDITOR_API FFXMeshDiscRingParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disc / Ring", meta = (UIMin = "1.0", UIMax = "500.0", ClampMin = "0.1", ClampMax = "10000.0"))
	float OuterRadius = 150.0f;

	/** Inner hole radius (0 for a solid disc, > 0 for a shockwave ring) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disc / Ring", meta = (UIMin = "0.0", UIMax = "500.0", ClampMin = "0.0", ClampMax = "10000.0"))
	float InnerRadius = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disc / Ring", meta = (UIMin = "15.0", UIMax = "360.0", ClampMin = "5.0", ClampMax = "360.0"))
	float ArcAngle = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disc / Ring", meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "256"))
	int32 RadialSegments = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disc / Ring", meta = (UIMin = "1", UIMax = "64", ClampMin = "1", ClampMax = "128"))
	int32 RingSegments = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disc / Ring")
	bool bUseHeightCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disc / Ring", meta = (EditCondition = "bUseHeightCurve", EditConditionHides, UIMin = "-200.0", UIMax = "200.0"))
	float HeightAmplitude = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Disc / Ring", meta = (EditCondition = "bUseHeightCurve", EditConditionHides))
	FRuntimeFloatCurve HeightProfileCurve;
};

USTRUCT(BlueprintType)
struct FXMESHGENERATOREDITOR_API FFXMeshDomeParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dome / Sphere", meta = (UIMin = "1.0", UIMax = "500.0", ClampMin = "0.1", ClampMax = "10000.0"))
	float Radius = 100.0f;

	/** Vertical sweep angle in degrees (90 = hemisphere, 180 = full sphere) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dome / Sphere", meta = (UIMin = "10.0", UIMax = "180.0", ClampMin = "5.0", ClampMax = "180.0"))
	float VerticalSweepAngle = 90.0f;

	/** Horizontal sweep angle in degrees (360 = full revolution) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dome / Sphere", meta = (UIMin = "15.0", UIMax = "360.0", ClampMin = "5.0", ClampMax = "360.0"))
	float HorizontalArcAngle = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dome / Sphere", meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "256"))
	int32 LongitudeSegments = 36;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dome / Sphere", meta = (UIMin = "2", UIMax = "64", ClampMin = "2", ClampMax = "128"))
	int32 LatitudeSegments = 14;

	/** Cap the open bottom rim when VerticalSweepAngle < 180 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dome / Sphere", meta = (EditCondition = "VerticalSweepAngle < 179.9"))
	bool bCapBase = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dome / Sphere")
	bool bUseRadiusCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dome / Sphere", meta = (EditCondition = "bUseRadiusCurve", EditConditionHides))
	FRuntimeFloatCurve RadiusProfileCurve;
};

USTRUCT(BlueprintType)
struct FXMESHGENERATOREDITOR_API FFXMeshRibbonParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ribbon", meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.1", ClampMax = "10000.0"))
	float Length = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ribbon", meta = (UIMin = "1.0", UIMax = "500.0", ClampMin = "0.1", ClampMax = "10000.0"))
	float Width = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ribbon", meta = (UIMin = "1", UIMax = "128", ClampMin = "1", ClampMax = "256"))
	int32 LengthSegments = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ribbon", meta = (UIMin = "1", UIMax = "32", ClampMin = "1", ClampMax = "64"))
	int32 WidthSegments = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ribbon")
	bool bUseWidthCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ribbon", meta = (EditCondition = "bUseWidthCurve", EditConditionHides))
	FRuntimeFloatCurve WidthProfileCurve;
};

USTRUCT(BlueprintType)
struct FXMESHGENERATOREDITOR_API FFXMeshCrossPlaneParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cross-Plane", meta = (UIMin = "1", UIMax = "6", ClampMin = "1", ClampMax = "12"))
	int32 PlaneCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cross-Plane", meta = (UIMin = "1.0", UIMax = "500.0", ClampMin = "0.1", ClampMax = "10000.0"))
	float Width = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cross-Plane", meta = (UIMin = "1.0", UIMax = "1000.0", ClampMin = "0.1", ClampMax = "10000.0"))
	float Height = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cross-Plane", meta = (UIMin = "1", UIMax = "32", ClampMin = "1", ClampMax = "64"))
	int32 WidthSegments = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cross-Plane", meta = (UIMin = "1", UIMax = "64", ClampMin = "1", ClampMax = "128"))
	int32 HeightSegments = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cross-Plane")
	bool bUseWidthCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cross-Plane", meta = (EditCondition = "bUseWidthCurve", EditConditionHides))
	FRuntimeFloatCurve WidthProfileCurve;
};

USTRUCT(BlueprintType)
struct FXMESHGENERATOREDITOR_API FFXMeshTorusParams
{
	GENERATED_BODY()

	/** Distance from the center of the torus to the center of the tube */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torus", meta = (UIMin = "1.0", UIMax = "500.0", ClampMin = "1.0", ClampMax = "10000.0"))
	float MajorRadius = 100.0f;

	/** Radius of the tube cross-section */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torus", meta = (UIMin = "0.5", UIMax = "250.0", ClampMin = "0.1", ClampMax = "5000.0"))
	float TubeRadius = 18.0f;

	/** Sweep angle around the ring in degrees (360 for a full closed ring) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torus", meta = (UIMin = "15.0", UIMax = "360.0", ClampMin = "5.0", ClampMax = "360.0"))
	float ArcAngle = 360.0f;

	/** Number of segments around the major ring */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torus", meta = (UIMin = "3", UIMax = "128", ClampMin = "3", ClampMax = "256"))
	int32 MajorSegments = 40;

	/** Number of segments around the tube cross-section */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torus", meta = (UIMin = "3", UIMax = "64", ClampMin = "3", ClampMax = "128"))
	int32 TubeSegments = 16;

	/** Cap the start end of an open torus arc (ArcAngle < 360) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torus", meta = (EditCondition = "ArcAngle < 359.9"))
	bool bCapStart = false;

	/** Cap the finish end of an open torus arc (ArcAngle < 360) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torus", meta = (EditCondition = "ArcAngle < 359.9"))
	bool bCapEnd = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torus")
	bool bUseTubeRadiusCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Torus", meta = (EditCondition = "bUseTubeRadiusCurve", EditConditionHides))
	FRuntimeFloatCurve TubeRadiusCurve;
};

/**
 * Serializable settings struct shared between the Modeling Mode tool, Recipe DataAssets, and AssetUserData.
 */
USTRUCT(BlueprintType)
struct FXMESHGENERATOREDITOR_API FFXMeshGeneratorSettings
{
	GENERATED_BODY()

	// --- Shape Selection & Per-Shape Parameters ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	EFXMeshShapeType ShapeType = EFXMeshShapeType::CylinderTube;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (EditCondition = "ShapeType == EFXMeshShapeType::CylinderTube", EditConditionHides))
	FFXMeshCylinderParams Cylinder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (EditCondition = "ShapeType == EFXMeshShapeType::Cone", EditConditionHides))
	FFXMeshConeParams Cone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (EditCondition = "ShapeType == EFXMeshShapeType::SlashArc", EditConditionHides))
	FFXMeshSlashArcParams SlashArc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (EditCondition = "ShapeType == EFXMeshShapeType::DiscRing", EditConditionHides))
	FFXMeshDiscRingParams DiscRing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (EditCondition = "ShapeType == EFXMeshShapeType::HemisphereDome", EditConditionHides))
	FFXMeshDomeParams Dome;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (EditCondition = "ShapeType == EFXMeshShapeType::Ribbon", EditConditionHides))
	FFXMeshRibbonParams Ribbon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (EditCondition = "ShapeType == EFXMeshShapeType::CrossPlane", EditConditionHides))
	FFXMeshCrossPlaneParams CrossPlane;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (EditCondition = "ShapeType == EFXMeshShapeType::Torus", EditConditionHides))
	FFXMeshTorusParams Torus;

	/** Flip triangle winding so normals face inward */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
	bool bInvertNormals = false;

	// --- UVs ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs")
	EFXMeshUVProjection UVProjection = EFXMeshUVProjection::ShapeDefault;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs", meta = (UIMin = "0.1", UIMax = "10.0", ClampMin = "0.01", ClampMax = "100.0"))
	float UVTilingU = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs", meta = (UIMin = "0.1", UIMax = "10.0", ClampMin = "0.01", ClampMax = "100.0"))
	float UVTilingV = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs", meta = (UIMin = "-2.0", UIMax = "2.0"))
	float UVOffsetU = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs", meta = (UIMin = "-2.0", UIMax = "2.0"))
	float UVOffsetV = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs", meta = (UIMin = "-180.0", UIMax = "180.0"))
	float UVRotation = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs")
	bool bFlipU = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs")
	bool bFlipV = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs")
	bool bSwapUV = false;

	/** Pack normalized (RadialDistance, VProgress) into UV1 for shader-driven animation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UVs")
	bool bGenerateExtraUV1 = true;

	// --- Vertex Colors ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex Colors")
	bool bEnableVertexColors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex Colors", meta = (EditCondition = "bEnableVertexColors", EditConditionHides))
	EFXMeshVertexColorSource RedChannel = EFXMeshVertexColorSource::AxisGradientU;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex Colors", meta = (EditCondition = "bEnableVertexColors", EditConditionHides))
	EFXMeshVertexColorSource GreenChannel = EFXMeshVertexColorSource::AxisGradientV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex Colors", meta = (EditCondition = "bEnableVertexColors", EditConditionHides))
	EFXMeshVertexColorSource BlueChannel = EFXMeshVertexColorSource::RadialDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex Colors", meta = (EditCondition = "bEnableVertexColors", EditConditionHides))
	EFXMeshVertexColorSource AlphaChannel = EFXMeshVertexColorSource::EdgeFalloffV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertex Colors", meta = (EditCondition = "bEnableVertexColors", EditConditionHides, UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	FLinearColor ConstantColor = FLinearColor::White;

	// --- Positioning ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positioning")
	EFXMeshPivotPreset PivotPreset = EFXMeshPivotPreset::Base;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Positioning")
	EFXMeshAxis UpAxis = EFXMeshAxis::Z;
};

/**
 * Base class for ordered procedural deformation modifiers in the FX Mesh modifier stack.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class FXMESHGENERATOREDITOR_API UFXMeshModifier : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Modifier")
	bool bEnabled = true;

	/** Optional falloff curve evaluated over 0..1 along the modifier axis */
	UPROPERTY(EditAnywhere, Category = "Modifier")
	bool bUseFalloffCurve = false;

	UPROPERTY(EditAnywhere, Category = "Modifier", meta = (EditCondition = "bUseFalloffCurve", EditConditionHides))
	FRuntimeFloatCurve FalloffCurve;

	virtual void Apply(UE::Geometry::FDynamicMesh3& Mesh, const FBox& Bounds) const {}

protected:
	float EvaluateFalloff(float T) const;
};

/**
 * Scales vertices radially along a chosen axis from BottomScale to TopScale.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Taper"))
class FXMESHGENERATOREDITOR_API UFXMeshTaperModifier : public UFXMeshModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Taper")
	EFXMeshAxis Axis = EFXMeshAxis::Z;

	UPROPERTY(EditAnywhere, Category = "Taper", meta = (UIMin = "0.0", UIMax = "3.0"))
	float BottomScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Taper", meta = (UIMin = "0.0", UIMax = "3.0"))
	float TopScale = 0.25f;

	virtual void Apply(UE::Geometry::FDynamicMesh3& Mesh, const FBox& Bounds) const override;
};

/**
 * Twists vertices around a chosen axis.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Twist"))
class FXMESHGENERATOREDITOR_API UFXMeshTwistModifier : public UFXMeshModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Twist")
	EFXMeshAxis Axis = EFXMeshAxis::Z;

	UPROPERTY(EditAnywhere, Category = "Twist", meta = (UIMin = "-720.0", UIMax = "720.0"))
	float TwistAngle = 180.0f;

	virtual void Apply(UE::Geometry::FDynamicMesh3& Mesh, const FBox& Bounds) const override;
};

/**
 * Bends the mesh along an arc around a chosen bend axis.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Bend"))
class FXMESHGENERATOREDITOR_API UFXMeshBendModifier : public UFXMeshModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Bend")
	EFXMeshAxis PrimaryAxis = EFXMeshAxis::Z;

	UPROPERTY(EditAnywhere, Category = "Bend")
	EFXMeshAxis BendDirection = EFXMeshAxis::X;

	UPROPERTY(EditAnywhere, Category = "Bend", meta = (UIMin = "-360.0", UIMax = "360.0"))
	float BendAngle = 90.0f;

	virtual void Apply(UE::Geometry::FDynamicMesh3& Mesh, const FBox& Bounds) const override;
};

/**
 * Applies a sinusoidal wave displacement along an axis.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Wave"))
class FXMESHGENERATOREDITOR_API UFXMeshWaveModifier : public UFXMeshModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Wave")
	EFXMeshAxis WaveAxis = EFXMeshAxis::Z;

	UPROPERTY(EditAnywhere, Category = "Wave")
	EFXMeshAxis DisplaceAxis = EFXMeshAxis::X;

	UPROPERTY(EditAnywhere, Category = "Wave", meta = (UIMin = "-200.0", UIMax = "200.0"))
	float Amplitude = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Wave", meta = (UIMin = "0.1", UIMax = "20.0"))
	float Frequency = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Wave", meta = (UIMin = "0.0", UIMax = "6.283"))
	float Phase = 0.0f;

	virtual void Apply(UE::Geometry::FDynamicMesh3& Mesh, const FBox& Bounds) const override;
};

/**
 * Applies 3D Perlin noise displacement to vertices.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Noise"))
class FXMESHGENERATOREDITOR_API UFXMeshNoiseModifier : public UFXMeshModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Noise", meta = (UIMin = "0.0", UIMax = "200.0"))
	float Amplitude = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Noise", meta = (UIMin = "0.001", UIMax = "0.2"))
	float Frequency = 0.03f;

	UPROPERTY(EditAnywhere, Category = "Noise")
	int32 Seed = 1337;

	/** Displace along vertex normals instead of world XYZ */
	UPROPERTY(EditAnywhere, Category = "Noise")
	bool bAlongNormals = true;

	virtual void Apply(UE::Geometry::FDynamicMesh3& Mesh, const FBox& Bounds) const override;
};

/**
 * Pushes vertices inward or outward along their vertex normals.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, meta = (DisplayName = "Inflate / Bulge"))
class FXMESHGENERATOREDITOR_API UFXMeshInflateModifier : public UFXMeshModifier
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Inflate", meta = (UIMin = "-200.0", UIMax = "200.0"))
	float Amount = 15.0f;

	/** Apply a smooth sine bulge along the Z axis (0 at ends, 1 in middle) */
	UPROPERTY(EditAnywhere, Category = "Inflate")
	bool bMidBulgeFalloff = true;

	virtual void Apply(UE::Geometry::FDynamicMesh3& Mesh, const FBox& Bounds) const override;
};
