// Copyright Epic Games, Inc. All Rights Reserved.

#include "FXMeshBuilder.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/MeshNormals.h"

using namespace UE::Geometry;

namespace
{
	struct FFXVertexAttr
	{
		float RawU = 0.0f;
		float RawV = 0.0f;
		float RadialNorm = 0.0f;
	};

	double EvaluateCurveOrDefault(bool bUseCurve, const FRuntimeFloatCurve& CurveProp, double T, double DefaultValue = 1.0)
	{
		if (bUseCurve)
		{
			const FRichCurve* Curve = CurveProp.GetRichCurveConst();
			if (Curve && Curve->GetNumKeys() > 0)
			{
				return static_cast<double>(Curve->Eval(static_cast<float>(FMath::Clamp(T, 0.0, 1.0))));
			}
		}
		return DefaultValue;
	}

	FVector2f TransformUV(const FFXMeshGeneratorSettings& Settings, float RawU, float RawV)
	{
		float U = RawU;
		float V = RawV;

		if (Settings.UVProjection == EFXMeshUVProjection::PolarRadial)
		{
			const float Angle = RawU * UE_TWO_PI;
			const float R = RawV * 0.5f;
			U = 0.5f + R * FMath::Cos(Angle);
			V = 0.5f + R * FMath::Sin(Angle);
		}

		if (Settings.bSwapUV)
		{
			Swap(U, V);
		}
		if (Settings.bFlipU)
		{
			U = 1.0f - U;
		}
		if (Settings.bFlipV)
		{
			V = 1.0f - V;
		}

		if (!FMath::IsNearlyZero(Settings.UVRotation))
		{
			const float Rad = FMath::DegreesToRadians(Settings.UVRotation);
			const float C = FMath::Cos(Rad);
			const float S = FMath::Sin(Rad);
			const float DU = U - 0.5f;
			const float DV = V - 0.5f;
			U = 0.5f + DU * C - DV * S;
			V = 0.5f + DU * S + DV * C;
		}

		U = U * Settings.UVTilingU + Settings.UVOffsetU;
		V = V * Settings.UVTilingV + Settings.UVOffsetV;
		return FVector2f(U, V);
	}

	float EvaluateColorChannel(EFXMeshVertexColorSource Source, const FFXVertexAttr& Attr, float ConstantVal)
	{
		switch (Source)
		{
		case EFXMeshVertexColorSource::Constant:
			return ConstantVal;
		case EFXMeshVertexColorSource::AxisGradientU:
			return FMath::Clamp(Attr.RawU, 0.0f, 1.0f);
		case EFXMeshVertexColorSource::AxisGradientV:
			return FMath::Clamp(Attr.RawV, 0.0f, 1.0f);
		case EFXMeshVertexColorSource::EdgeFalloffU:
			return FMath::Sin(UE_PI * FMath::Clamp(Attr.RawU, 0.0f, 1.0f));
		case EFXMeshVertexColorSource::EdgeFalloffV:
			return FMath::Sin(UE_PI * FMath::Clamp(Attr.RawV, 0.0f, 1.0f));
		case EFXMeshVertexColorSource::RadialDistance:
			return FMath::Clamp(Attr.RadialNorm, 0.0f, 1.0f);
		default:
			return ConstantVal;
		}
	}

	FVector4f ComputeVertexColor(const FFXMeshGeneratorSettings& Settings, const FFXVertexAttr& Attr)
	{
		return FVector4f(
			EvaluateColorChannel(Settings.RedChannel, Attr, Settings.ConstantColor.R),
			EvaluateColorChannel(Settings.GreenChannel, Attr, Settings.ConstantColor.G),
			EvaluateColorChannel(Settings.BlueChannel, Attr, Settings.ConstantColor.B),
			EvaluateColorChannel(Settings.AlphaChannel, Attr, Settings.ConstantColor.A));
	}

	struct FMeshBuildContext
	{
		FDynamicMesh3& Mesh;
		const FFXMeshGeneratorSettings& Settings;
		FDynamicMeshUVOverlay* UV0 = nullptr;
		FDynamicMeshUVOverlay* UV1 = nullptr;
		FDynamicMeshColorOverlay* ColorOverlay = nullptr;

		FMeshBuildContext(FDynamicMesh3& InMesh, const FFXMeshGeneratorSettings& InSettings)
			: Mesh(InMesh), Settings(InSettings)
		{
			Mesh.Clear();
			Mesh.EnableTriangleGroups();
			Mesh.EnableAttributes();
			Mesh.Attributes()->SetNumUVLayers(Settings.bGenerateExtraUV1 ? 2 : 1);
			UV0 = Mesh.Attributes()->GetUVLayer(0);
			if (Settings.bGenerateExtraUV1)
			{
				UV1 = Mesh.Attributes()->GetUVLayer(1);
			}
			if (Settings.bEnableVertexColors)
			{
				Mesh.Attributes()->EnablePrimaryColors();
				ColorOverlay = Mesh.Attributes()->PrimaryColors();
			}
		}

		int32 AddUV0(float RawU, float RawV) const
		{
			return UV0->AppendElement(TransformUV(Settings, RawU, RawV));
		}

		int32 AddUV1(float RadialNorm, float RawV) const
		{
			return UV1 ? UV1->AppendElement(FVector2f(RadialNorm, RawV)) : INDEX_NONE;
		}

		int32 AddColor(const FFXVertexAttr& Attr) const
		{
			return ColorOverlay ? ColorOverlay->AppendElement(ComputeVertexColor(Settings, Attr)) : INDEX_NONE;
		}

		void AddTri(
			int32 V0, int32 V1, int32 V2,
			int32 U0, int32 U1, int32 U2,
			int32 U1_0, int32 U1_1, int32 U1_2,
			int32 C0, int32 C1, int32 C2,
			int32 GroupId = 0)
		{
			if (Settings.bInvertNormals)
			{
				Swap(V1, V2);
				Swap(U1, U2);
				Swap(U1_1, U1_2);
				Swap(C1, C2);
			}

			const int32 Tid = Mesh.AppendTriangle(V0, V1, V2, GroupId);
			if (Tid >= 0)
			{
				UV0->SetTriangle(Tid, FIndex3i(U0, U1, U2));
				if (UV1 && U1_0 >= 0 && U1_1 >= 0 && U1_2 >= 0)
				{
					UV1->SetTriangle(Tid, FIndex3i(U1_0, U1_1, U1_2));
				}
				if (ColorOverlay && C0 >= 0 && C1 >= 0 && C2 >= 0)
				{
					ColorOverlay->SetTriangle(Tid, FIndex3i(C0, C1, C2));
				}
			}
		}

		void AddQuad(
			int32 V00, int32 V10, int32 V01, int32 V11,
			int32 U00, int32 U10, int32 U01, int32 U11,
			int32 U1_00, int32 U1_10, int32 U1_01, int32 U1_11,
			int32 C00, int32 C10, int32 C01, int32 C11,
			int32 GroupId = 0)
		{
			AddTri(V00, V01, V11, U00, U01, U11, U1_00, U1_01, U1_11, C00, C01, C11, GroupId);
			AddTri(V00, V11, V10, U00, U11, U10, U1_00, U1_11, U1_10, C00, C11, C10, GroupId);
		}
	};

	void BuildRevolvedTube(
		FMeshBuildContext& Ctx,
		double BottomR, double TopR, double Height, double ArcDeg,
		int32 RadialSegs, int32 HeightSegs,
		bool bCapStart, bool bCapEnd,
		bool bUseCurve, const FRuntimeFloatCurve& Curve)
	{
		RadialSegs = FMath::Clamp(RadialSegs, 3, 256);
		HeightSegs = FMath::Clamp(HeightSegs, 1, 128);
		ArcDeg = FMath::Clamp(ArcDeg, 5.0, 360.0);
		const bool bClosedRing = FMath::IsNearlyEqual(ArcDeg, 360.0, 0.01);

		const int32 RingVertCount = bClosedRing ? RadialSegs : (RadialSegs + 1);
		const int32 UVCols = RadialSegs + 1;
		const int32 Rows = HeightSegs + 1;

		TArray<int32> VertGrid;
		VertGrid.Reserve(RingVertCount * Rows);
		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const double V = static_cast<double>(Row) / static_cast<double>(HeightSegs);
			const double Profile = EvaluateCurveOrDefault(bUseCurve, Curve, V, 1.0);
			const double Radius = FMath::Max(0.01, FMath::Lerp(BottomR, TopR, V) * Profile);
			const double Z = V * Height;

			for (int32 Col = 0; Col < RingVertCount; ++Col)
			{
				const double U = static_cast<double>(Col) / static_cast<double>(RadialSegs);
				const double AngleRad = FMath::DegreesToRadians(U * ArcDeg);
				VertGrid.Add(Ctx.Mesh.AppendVertex(FVector3d(Radius * FMath::Cos(AngleRad), Radius * FMath::Sin(AngleRad), Z)));
			}
		}

		TArray<int32> UV0Grid, UV1Grid, ColorGrid;
		UV0Grid.Reserve(UVCols * Rows);
		UV1Grid.Reserve(UVCols * Rows);
		ColorGrid.Reserve(UVCols * Rows);

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const float RawV = static_cast<float>(Row) / static_cast<float>(HeightSegs);
			for (int32 Col = 0; Col < UVCols; ++Col)
			{
				const float RawU = static_cast<float>(Col) / static_cast<float>(RadialSegs);
				const FFXVertexAttr Attr{ RawU, RawV, 1.0f };
				UV0Grid.Add(Ctx.AddUV0(RawU, RawV));
				UV1Grid.Add(Ctx.AddUV1(Attr.RadialNorm, RawV));
				ColorGrid.Add(Ctx.AddColor(Attr));
			}
		}

		for (int32 Row = 0; Row < HeightSegs; ++Row)
		{
			for (int32 Col = 0; Col < RadialSegs; ++Col)
			{
				const int32 NextColPos = bClosedRing ? ((Col + 1) % RadialSegs) : (Col + 1);
				const int32 NextColUV = Col + 1;

				const int32 I00 = Row * RingVertCount + Col;
				const int32 I10 = Row * RingVertCount + NextColPos;
				const int32 I01 = (Row + 1) * RingVertCount + Col;
				const int32 I11 = (Row + 1) * RingVertCount + NextColPos;

				const int32 U00 = Row * UVCols + Col;
				const int32 U10 = Row * UVCols + NextColUV;
				const int32 U01 = (Row + 1) * UVCols + Col;
				const int32 U11 = (Row + 1) * UVCols + NextColUV;

				Ctx.AddQuad(
					VertGrid[I00], VertGrid[I10], VertGrid[I01], VertGrid[I11],
					UV0Grid[U00], UV0Grid[U10], UV0Grid[U01], UV0Grid[U11],
					UV1Grid[U00], UV1Grid[U10], UV1Grid[U01], UV1Grid[U11],
					ColorGrid[U00], ColorGrid[U10], ColorGrid[U01], ColorGrid[U11], 0);
			}
		}

		auto AddCap = [&](int32 RowIdx, double ZPos, bool bFlipWinding, int32 GroupId)
		{
			const float RawV = (RowIdx == 0) ? 0.0f : 1.0f;
			const int32 CenterV = Ctx.Mesh.AppendVertex(FVector3d(0.0, 0.0, ZPos));
			const FFXVertexAttr CenterAttr{ 0.5f, RawV, 0.0f };
			const int32 CenterUV0 = Ctx.AddUV0(0.5f, 0.5f);
			const int32 CenterUV1 = Ctx.AddUV1(0.0f, RawV);
			const int32 CenterCol = Ctx.AddColor(CenterAttr);

			for (int32 Col = 0; Col < RadialSegs; ++Col)
			{
				const int32 NextColPos = bClosedRing ? ((Col + 1) % RadialSegs) : (Col + 1);
				const int32 V1 = VertGrid[RowIdx * RingVertCount + Col];
				const int32 V2 = VertGrid[RowIdx * RingVertCount + NextColPos];

				const double A1 = FMath::DegreesToRadians((static_cast<double>(Col) / RadialSegs) * ArcDeg);
				const double A2 = FMath::DegreesToRadians((static_cast<double>(Col + 1) / RadialSegs) * ArcDeg);
				const int32 U1 = Ctx.AddUV0(0.5f + 0.5f * FMath::Cos(A1), 0.5f + 0.5f * FMath::Sin(A1));
				const int32 U2 = Ctx.AddUV0(0.5f + 0.5f * FMath::Cos(A2), 0.5f + 0.5f * FMath::Sin(A2));
				const int32 U1_1 = Ctx.AddUV1(1.0f, RawV);
				const int32 U1_2 = Ctx.AddUV1(1.0f, RawV);
				const int32 C1 = Ctx.AddColor(FFXVertexAttr{ static_cast<float>(Col) / RadialSegs, RawV, 1.0f });
				const int32 C2 = Ctx.AddColor(FFXVertexAttr{ static_cast<float>(Col + 1) / RadialSegs, RawV, 1.0f });

				if (bFlipWinding)
				{
					Ctx.AddTri(CenterV, V1, V2, CenterUV0, U1, U2, CenterUV1, U1_1, U1_2, CenterCol, C1, C2, GroupId);
				}
				else
				{
					Ctx.AddTri(CenterV, V2, V1, CenterUV0, U2, U1, CenterUV1, U1_2, U1_1, CenterCol, C2, C1, GroupId);
				}
			}
		};

		if (bCapStart && BottomR > 0.05)
		{
			AddCap(0, 0.0, true, 1);
		}
		if (bCapEnd && TopR > 0.05)
		{
			AddCap(HeightSegs, Height, false, 2);
		}
	}

	void BuildSlashArc(FMeshBuildContext& Ctx)
	{
		const FFXMeshSlashArcParams& P = Ctx.Settings.SlashArc;
		const int32 ArcSegs = FMath::Clamp(P.ArcSegments, 3, 256);
		const int32 CrossSegs = FMath::Clamp(P.CrossSegments, 1, 64);
		const double ArcDeg = FMath::Clamp(static_cast<double>(P.ArcAngle), 5.0, 360.0);
		const bool bClosedRing = FMath::IsNearlyEqual(ArcDeg, 360.0, 0.01);

		const double OuterR = FMath::Max(1.0, static_cast<double>(P.OuterRadius));
		const double InnerR = FMath::Clamp(static_cast<double>(P.InnerRadius), 0.0, OuterR - 0.1);
		const double ArchHeight = static_cast<double>(P.ArchHeight);

		const int32 ColVerts = bClosedRing ? ArcSegs : (ArcSegs + 1);
		const int32 UVCols = ArcSegs + 1;
		const int32 Rows = CrossSegs + 1;

		TArray<int32> VertGrid;
		VertGrid.Reserve(ColVerts * Rows);

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const double V = static_cast<double>(Row) / static_cast<double>(CrossSegs);
			for (int32 Col = 0; Col < ColVerts; ++Col)
			{
				const double U = static_cast<double>(Col) / static_cast<double>(ArcSegs);
				const double TipTaper = (bClosedRing || !P.bTaperTips) ? 1.0 : FMath::Sin(UE_DOUBLE_PI * U);
				const double Profile = EvaluateCurveOrDefault(P.bUseWidthCurve, P.WidthProfileCurve, U, 1.0);
				const double LocalInner = FMath::Lerp(OuterR, InnerR, FMath::Clamp(TipTaper * Profile, 0.0, 2.0));
				const double R = FMath::Lerp(LocalInner, OuterR, V);
				const double AngleRad = FMath::DegreesToRadians((U - 0.5) * ArcDeg);
				const double Z = FMath::Sin(UE_DOUBLE_PI * V) * ArchHeight * TipTaper;

				VertGrid.Add(Ctx.Mesh.AppendVertex(FVector3d(R * FMath::Cos(AngleRad), R * FMath::Sin(AngleRad), Z)));
			}
		}

		TArray<int32> UV0Grid, UV1Grid, ColorGrid;
		UV0Grid.Reserve(UVCols * Rows);
		UV1Grid.Reserve(UVCols * Rows);
		ColorGrid.Reserve(UVCols * Rows);

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const float RawV = static_cast<float>(Row) / static_cast<float>(CrossSegs);
			for (int32 Col = 0; Col < UVCols; ++Col)
			{
				const float RawU = static_cast<float>(Col) / static_cast<float>(ArcSegs);
				const FFXVertexAttr Attr{ RawU, RawV, RawV };
				UV0Grid.Add(Ctx.AddUV0(RawU, RawV));
				UV1Grid.Add(Ctx.AddUV1(RawV, RawU));
				ColorGrid.Add(Ctx.AddColor(Attr));
			}
		}

		for (int32 Row = 0; Row < CrossSegs; ++Row)
		{
			for (int32 Col = 0; Col < ArcSegs; ++Col)
			{
				const int32 NextColPos = bClosedRing ? ((Col + 1) % ArcSegs) : (Col + 1);
				const int32 NextColUV = Col + 1;

				const int32 I00 = Row * ColVerts + Col;
				const int32 I10 = Row * ColVerts + NextColPos;
				const int32 I01 = (Row + 1) * ColVerts + Col;
				const int32 I11 = (Row + 1) * ColVerts + NextColPos;

				const int32 U00 = Row * UVCols + Col;
				const int32 U10 = Row * UVCols + NextColUV;
				const int32 U01 = (Row + 1) * UVCols + Col;
				const int32 U11 = (Row + 1) * UVCols + NextColUV;

				Ctx.AddQuad(
					VertGrid[I00], VertGrid[I01], VertGrid[I10], VertGrid[I11],
					UV0Grid[U00], UV0Grid[U01], UV0Grid[U10], UV0Grid[U11],
					UV1Grid[U00], UV1Grid[U01], UV1Grid[U10], UV1Grid[U11],
					ColorGrid[U00], ColorGrid[U01], ColorGrid[U10], ColorGrid[U11], 0);
			}
		}
	}

	void BuildDiscRing(FMeshBuildContext& Ctx)
	{
		const FFXMeshDiscRingParams& P = Ctx.Settings.DiscRing;
		const int32 RadialSegs = FMath::Clamp(P.RadialSegments, 3, 256);
		const int32 RingSegs = FMath::Clamp(P.RingSegments, 1, 128);
		const double ArcDeg = FMath::Clamp(static_cast<double>(P.ArcAngle), 5.0, 360.0);
		const bool bClosedRing = FMath::IsNearlyEqual(ArcDeg, 360.0, 0.01);

		const double OuterR = FMath::Max(0.1, static_cast<double>(P.OuterRadius));
		const double InnerR = FMath::Clamp(static_cast<double>(P.InnerRadius), 0.0, OuterR);

		const int32 ColVerts = bClosedRing ? RadialSegs : (RadialSegs + 1);
		const int32 UVCols = RadialSegs + 1;
		const int32 Rows = RingSegs + 1;

		TArray<int32> VertGrid;
		VertGrid.Reserve(ColVerts * Rows);

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const double V = static_cast<double>(Row) / static_cast<double>(RingSegs);
			const double R = FMath::Lerp(InnerR, OuterR, V);
			const double Z = P.bUseHeightCurve
				? EvaluateCurveOrDefault(true, P.HeightProfileCurve, V, 0.0) * static_cast<double>(P.HeightAmplitude)
				: 0.0;

			for (int32 Col = 0; Col < ColVerts; ++Col)
			{
				const double U = static_cast<double>(Col) / static_cast<double>(RadialSegs);
				const double AngleRad = FMath::DegreesToRadians(U * ArcDeg);
				VertGrid.Add(Ctx.Mesh.AppendVertex(FVector3d(R * FMath::Cos(AngleRad), R * FMath::Sin(AngleRad), Z)));
			}
		}

		TArray<int32> UV0Grid, UV1Grid, ColorGrid;
		UV0Grid.Reserve(UVCols * Rows);
		UV1Grid.Reserve(UVCols * Rows);
		ColorGrid.Reserve(UVCols * Rows);

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const float RawV = static_cast<float>(Row) / static_cast<float>(RingSegs);
			for (int32 Col = 0; Col < UVCols; ++Col)
			{
				const float RawU = static_cast<float>(Col) / static_cast<float>(RadialSegs);
				const FFXVertexAttr Attr{ RawU, RawV, RawV };
				UV0Grid.Add(Ctx.AddUV0(RawU, RawV));
				UV1Grid.Add(Ctx.AddUV1(RawV, RawU));
				ColorGrid.Add(Ctx.AddColor(Attr));
			}
		}

		for (int32 Row = 0; Row < RingSegs; ++Row)
		{
			for (int32 Col = 0; Col < RadialSegs; ++Col)
			{
				const int32 NextColPos = bClosedRing ? ((Col + 1) % RadialSegs) : (Col + 1);
				const int32 NextColUV = Col + 1;

				const int32 I00 = Row * ColVerts + Col;
				const int32 I10 = Row * ColVerts + NextColPos;
				const int32 I01 = (Row + 1) * ColVerts + Col;
				const int32 I11 = (Row + 1) * ColVerts + NextColPos;

				const int32 U00 = Row * UVCols + Col;
				const int32 U10 = Row * UVCols + NextColUV;
				const int32 U01 = (Row + 1) * UVCols + Col;
				const int32 U11 = (Row + 1) * UVCols + NextColUV;

				Ctx.AddQuad(
					VertGrid[I00], VertGrid[I01], VertGrid[I10], VertGrid[I11],
					UV0Grid[U00], UV0Grid[U01], UV0Grid[U10], UV0Grid[U11],
					UV1Grid[U00], UV1Grid[U01], UV1Grid[U10], UV1Grid[U11],
					ColorGrid[U00], ColorGrid[U01], ColorGrid[U10], ColorGrid[U11], 0);
			}
		}
	}

	void BuildHemisphereDome(FMeshBuildContext& Ctx)
	{
		const FFXMeshDomeParams& P = Ctx.Settings.Dome;
		const int32 LonSegs = FMath::Clamp(P.LongitudeSegments, 3, 256);
		const int32 LatSegs = FMath::Clamp(P.LatitudeSegments, 2, 128);
		const double ArcDeg = FMath::Clamp(static_cast<double>(P.HorizontalArcAngle), 5.0, 360.0);
		const double SweepDeg = FMath::Clamp(static_cast<double>(P.VerticalSweepAngle), 5.0, 180.0);
		const bool bClosedRing = FMath::IsNearlyEqual(ArcDeg, 360.0, 0.01);

		const double BaseRadius = FMath::Max(0.1, static_cast<double>(P.Radius));
		const int32 ColVerts = bClosedRing ? LonSegs : (LonSegs + 1);
		const int32 UVCols = LonSegs + 1;
		const int32 Rows = LatSegs + 1;

		TArray<int32> VertGrid;
		VertGrid.Reserve(ColVerts * Rows);

		const double MaxPhi = FMath::DegreesToRadians(SweepDeg);
		const double MinZ = BaseRadius * FMath::Cos(MaxPhi);

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const double V = static_cast<double>(Row) / static_cast<double>(LatSegs);
			const double Phi = FMath::Lerp(MaxPhi, 0.001, V);
			const double Profile = EvaluateCurveOrDefault(P.bUseRadiusCurve, P.RadiusProfileCurve, V, 1.0);
			const double R = BaseRadius * FMath::Sin(Phi) * Profile;
			const double Z = BaseRadius * FMath::Cos(Phi) - MinZ;

			for (int32 Col = 0; Col < ColVerts; ++Col)
			{
				const double U = static_cast<double>(Col) / static_cast<double>(LonSegs);
				const double Theta = FMath::DegreesToRadians(U * ArcDeg);
				VertGrid.Add(Ctx.Mesh.AppendVertex(FVector3d(R * FMath::Cos(Theta), R * FMath::Sin(Theta), Z)));
			}
		}

		TArray<int32> UV0Grid, UV1Grid, ColorGrid;
		UV0Grid.Reserve(UVCols * Rows);
		UV1Grid.Reserve(UVCols * Rows);
		ColorGrid.Reserve(UVCols * Rows);

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const float RawV = static_cast<float>(Row) / static_cast<float>(LatSegs);
			const float RadialNorm = 1.0f - RawV;
			for (int32 Col = 0; Col < UVCols; ++Col)
			{
				const float RawU = static_cast<float>(Col) / static_cast<float>(LonSegs);
				const FFXVertexAttr Attr{ RawU, RawV, RadialNorm };
				UV0Grid.Add(Ctx.AddUV0(RawU, RawV));
				UV1Grid.Add(Ctx.AddUV1(RadialNorm, RawV));
				ColorGrid.Add(Ctx.AddColor(Attr));
			}
		}

		for (int32 Row = 0; Row < LatSegs; ++Row)
		{
			for (int32 Col = 0; Col < LonSegs; ++Col)
			{
				const int32 NextColPos = bClosedRing ? ((Col + 1) % LonSegs) : (Col + 1);
				const int32 NextColUV = Col + 1;

				const int32 I00 = Row * ColVerts + Col;
				const int32 I10 = Row * ColVerts + NextColPos;
				const int32 I01 = (Row + 1) * ColVerts + Col;
				const int32 I11 = (Row + 1) * ColVerts + NextColPos;

				const int32 U00 = Row * UVCols + Col;
				const int32 U10 = Row * UVCols + NextColUV;
				const int32 U01 = (Row + 1) * UVCols + Col;
				const int32 U11 = (Row + 1) * UVCols + NextColUV;

				Ctx.AddQuad(
					VertGrid[I00], VertGrid[I10], VertGrid[I01], VertGrid[I11],
					UV0Grid[U00], UV0Grid[U10], UV0Grid[U01], UV0Grid[U11],
					UV1Grid[U00], UV1Grid[U10], UV1Grid[U01], UV1Grid[U11],
					ColorGrid[U00], ColorGrid[U10], ColorGrid[U01], ColorGrid[U11], 0);
			}
		}

		// Optional Base Cap when dome is open at bottom
		if (P.bCapBase && SweepDeg < 179.9)
		{
			const int32 CenterV = Ctx.Mesh.AppendVertex(FVector3d(0.0, 0.0, 0.0));
			const int32 CenterUV0 = Ctx.AddUV0(0.5f, 0.5f);
			const int32 CenterUV1 = Ctx.AddUV1(0.0f, 0.0f);
			const int32 CenterCol = Ctx.AddColor(FFXVertexAttr{ 0.5f, 0.0f, 0.0f });

			for (int32 Col = 0; Col < LonSegs; ++Col)
			{
				const int32 NextColPos = bClosedRing ? ((Col + 1) % LonSegs) : (Col + 1);
				const int32 V1 = VertGrid[Col];
				const int32 V2 = VertGrid[NextColPos];

				const double A1 = FMath::DegreesToRadians((static_cast<double>(Col) / LonSegs) * ArcDeg);
				const double A2 = FMath::DegreesToRadians((static_cast<double>(Col + 1) / LonSegs) * ArcDeg);
				const int32 U1 = Ctx.AddUV0(0.5f + 0.5f * FMath::Cos(A1), 0.5f + 0.5f * FMath::Sin(A1));
				const int32 U2 = Ctx.AddUV0(0.5f + 0.5f * FMath::Cos(A2), 0.5f + 0.5f * FMath::Sin(A2));
				const int32 U1_1 = Ctx.AddUV1(1.0f, 0.0f);
				const int32 U1_2 = Ctx.AddUV1(1.0f, 0.0f);
				const int32 C1 = Ctx.AddColor(FFXVertexAttr{ static_cast<float>(Col) / LonSegs, 0.0f, 1.0f });
				const int32 C2 = Ctx.AddColor(FFXVertexAttr{ static_cast<float>(Col + 1) / LonSegs, 0.0f, 1.0f });

				Ctx.AddTri(CenterV, V1, V2, CenterUV0, U1, U2, CenterUV1, U1_1, U1_2, CenterCol, C1, C2, 1);
			}
		}
	}

	void BuildRibbon(FMeshBuildContext& Ctx)
	{
		const FFXMeshRibbonParams& P = Ctx.Settings.Ribbon;
		const int32 WidthSegs = FMath::Clamp(P.WidthSegments, 1, 64);
		const int32 LengthSegs = FMath::Clamp(P.LengthSegments, 1, 256);
		const double Length = FMath::Max(1.0, static_cast<double>(P.Length));
		const double HalfW = FMath::Max(0.5, static_cast<double>(P.Width) * 0.5);

		const int32 Cols = WidthSegs + 1;
		const int32 Rows = LengthSegs + 1;

		TArray<int32> VertGrid, UV0Grid, UV1Grid, ColorGrid;
		VertGrid.Reserve(Cols * Rows);
		UV0Grid.Reserve(Cols * Rows);
		UV1Grid.Reserve(Cols * Rows);
		ColorGrid.Reserve(Cols * Rows);

		for (int32 Row = 0; Row < Rows; ++Row)
		{
			const double V = static_cast<double>(Row) / static_cast<double>(LengthSegs);
			const double Profile = EvaluateCurveOrDefault(P.bUseWidthCurve, P.WidthProfileCurve, V, 1.0);
			const double Z = V * Length;

			for (int32 Col = 0; Col < Cols; ++Col)
			{
				const double U = static_cast<double>(Col) / static_cast<double>(WidthSegs);
				const double X = (U * 2.0 - 1.0) * HalfW * Profile;
				const float RadialNorm = FMath::Abs(static_cast<float>(U * 2.0 - 1.0));
				const FFXVertexAttr Attr{ static_cast<float>(U), static_cast<float>(V), RadialNorm };

				VertGrid.Add(Ctx.Mesh.AppendVertex(FVector3d(X, 0.0, Z)));
				UV0Grid.Add(Ctx.AddUV0(Attr.RawU, Attr.RawV));
				UV1Grid.Add(Ctx.AddUV1(RadialNorm, Attr.RawV));
				ColorGrid.Add(Ctx.AddColor(Attr));
			}
		}

		for (int32 Row = 0; Row < LengthSegs; ++Row)
		{
			for (int32 Col = 0; Col < WidthSegs; ++Col)
			{
				const int32 I00 = Row * Cols + Col;
				const int32 I10 = Row * Cols + Col + 1;
				const int32 I01 = (Row + 1) * Cols + Col;
				const int32 I11 = (Row + 1) * Cols + Col + 1;

				Ctx.AddQuad(
					VertGrid[I00], VertGrid[I10], VertGrid[I01], VertGrid[I11],
					UV0Grid[I00], UV0Grid[I10], UV0Grid[I01], UV0Grid[I11],
					UV1Grid[I00], UV1Grid[I10], UV1Grid[I01], UV1Grid[I11],
					ColorGrid[I00], ColorGrid[I10], ColorGrid[I01], ColorGrid[I11], 0);
			}
		}
	}

	void BuildCrossPlane(FMeshBuildContext& Ctx)
	{
		const FFXMeshCrossPlaneParams& P = Ctx.Settings.CrossPlane;
		const int32 PlaneCount = FMath::Clamp(P.PlaneCount, 1, 12);
		const int32 WidthSegs = FMath::Clamp(P.WidthSegments, 1, 64);
		const int32 HeightSegs = FMath::Clamp(P.HeightSegments, 1, 128);
		const double Height = FMath::Max(1.0, static_cast<double>(P.Height));
		const double HalfW = FMath::Max(0.5, static_cast<double>(P.Width) * 0.5);

		const int32 Cols = WidthSegs + 1;
		const int32 Rows = HeightSegs + 1;

		for (int32 PlaneIdx = 0; PlaneIdx < PlaneCount; ++PlaneIdx)
		{
			const double AngleRad = (UE_DOUBLE_PI * static_cast<double>(PlaneIdx)) / static_cast<double>(PlaneCount);
			const double DirX = FMath::Cos(AngleRad);
			const double DirY = FMath::Sin(AngleRad);

			TArray<int32> VertGrid, UV0Grid, UV1Grid, ColorGrid;
			VertGrid.Reserve(Cols * Rows);
			UV0Grid.Reserve(Cols * Rows);
			UV1Grid.Reserve(Cols * Rows);
			ColorGrid.Reserve(Cols * Rows);

			for (int32 Row = 0; Row < Rows; ++Row)
			{
				const double V = static_cast<double>(Row) / static_cast<double>(HeightSegs);
				const double Profile = EvaluateCurveOrDefault(P.bUseWidthCurve, P.WidthProfileCurve, V, 1.0);
				const double Z = V * Height;

				for (int32 Col = 0; Col < Cols; ++Col)
				{
					const double U = static_cast<double>(Col) / static_cast<double>(WidthSegs);
					const double Along = (U * 2.0 - 1.0) * HalfW * Profile;
					const float RadialNorm = FMath::Abs(static_cast<float>(U * 2.0 - 1.0));
					const FFXVertexAttr Attr{ static_cast<float>(U), static_cast<float>(V), RadialNorm };

					VertGrid.Add(Ctx.Mesh.AppendVertex(FVector3d(Along * DirX, Along * DirY, Z)));
					UV0Grid.Add(Ctx.AddUV0(Attr.RawU, Attr.RawV));
					UV1Grid.Add(Ctx.AddUV1(RadialNorm, Attr.RawV));
					ColorGrid.Add(Ctx.AddColor(Attr));
				}
			}

			for (int32 Row = 0; Row < HeightSegs; ++Row)
			{
				for (int32 Col = 0; Col < WidthSegs; ++Col)
				{
					const int32 I00 = Row * Cols + Col;
					const int32 I10 = Row * Cols + Col + 1;
					const int32 I01 = (Row + 1) * Cols + Col;
					const int32 I11 = (Row + 1) * Cols + Col + 1;

					Ctx.AddQuad(
						VertGrid[I00], VertGrid[I10], VertGrid[I01], VertGrid[I11],
						UV0Grid[I00], UV0Grid[I10], UV0Grid[I01], UV0Grid[I11],
						UV1Grid[I00], UV1Grid[I10], UV1Grid[I01], UV1Grid[I11],
						ColorGrid[I00], ColorGrid[I10], ColorGrid[I01], ColorGrid[I11], PlaneIdx);
				}
			}
		}
	}

	void BuildTorus(FMeshBuildContext& Ctx)
	{
		const FFXMeshTorusParams& P = Ctx.Settings.Torus;
		const int32 MajorSegs = FMath::Clamp(P.MajorSegments, 3, 256);
		const int32 MinorSegs = FMath::Clamp(P.TubeSegments, 3, 128);
		const double ArcDeg = FMath::Clamp(static_cast<double>(P.ArcAngle), 5.0, 360.0);
		const bool bClosedMajor = FMath::IsNearlyEqual(ArcDeg, 360.0, 0.01);

		const double MajorR = FMath::Max(1.0, static_cast<double>(P.MajorRadius));
		const double MinorR = FMath::Max(0.1, static_cast<double>(P.TubeRadius));

		const int32 MajorVerts = bClosedMajor ? MajorSegs : (MajorSegs + 1);
		const int32 UVCols = MajorSegs + 1;
		const int32 UVRows = MinorSegs + 1;

		TArray<int32> VertGrid;
		VertGrid.Reserve(MajorVerts * MinorSegs);

		for (int32 Row = 0; Row < MinorSegs; ++Row)
		{
			const double V = static_cast<double>(Row) / static_cast<double>(MinorSegs);
			const double MinorAngle = V * UE_DOUBLE_TWO_PI;
			const double CosMin = FMath::Cos(MinorAngle);
			const double SinMin = FMath::Sin(MinorAngle);

			for (int32 Col = 0; Col < MajorVerts; ++Col)
			{
				const double U = static_cast<double>(Col) / static_cast<double>(MajorSegs);
				const double Profile = EvaluateCurveOrDefault(P.bUseTubeRadiusCurve, P.TubeRadiusCurve, U, 1.0);
				const double LocalMinorR = MinorR * Profile;
				const double MajorAngle = FMath::DegreesToRadians(U * ArcDeg);
				const double R = MajorR + LocalMinorR * CosMin;
				const double Z = LocalMinorR * SinMin;

				VertGrid.Add(Ctx.Mesh.AppendVertex(FVector3d(R * FMath::Cos(MajorAngle), R * FMath::Sin(MajorAngle), Z)));
			}
		}

		TArray<int32> UV0Grid, UV1Grid, ColorGrid;
		UV0Grid.Reserve(UVCols * UVRows);
		UV1Grid.Reserve(UVCols * UVRows);
		ColorGrid.Reserve(UVCols * UVRows);

		for (int32 Row = 0; Row < UVRows; ++Row)
		{
			const float RawV = static_cast<float>(Row) / static_cast<float>(MinorSegs);
			const float RadialNorm = 0.5f + 0.5f * FMath::Cos(RawV * UE_TWO_PI);
			for (int32 Col = 0; Col < UVCols; ++Col)
			{
				const float RawU = static_cast<float>(Col) / static_cast<float>(MajorSegs);
				const FFXVertexAttr Attr{ RawU, RawV, RadialNorm };
				UV0Grid.Add(Ctx.AddUV0(RawU, RawV));
				UV1Grid.Add(Ctx.AddUV1(RadialNorm, RawV));
				ColorGrid.Add(Ctx.AddColor(Attr));
			}
		}

		for (int32 Row = 0; Row < MinorSegs; ++Row)
		{
			const int32 NextRowPos = (Row + 1) % MinorSegs;
			const int32 NextRowUV = Row + 1;

			for (int32 Col = 0; Col < MajorSegs; ++Col)
			{
				const int32 NextColPos = bClosedMajor ? ((Col + 1) % MajorSegs) : (Col + 1);
				const int32 NextColUV = Col + 1;

				const int32 I00 = Row * MajorVerts + Col;
				const int32 I10 = Row * MajorVerts + NextColPos;
				const int32 I01 = NextRowPos * MajorVerts + Col;
				const int32 I11 = NextRowPos * MajorVerts + NextColPos;

				const int32 U00 = Row * UVCols + Col;
				const int32 U10 = Row * UVCols + NextColUV;
				const int32 U01 = NextRowUV * UVCols + Col;
				const int32 U11 = NextRowUV * UVCols + NextColUV;

				Ctx.AddQuad(
					VertGrid[I00], VertGrid[I10], VertGrid[I01], VertGrid[I11],
					UV0Grid[U00], UV0Grid[U10], UV0Grid[U01], UV0Grid[U11],
					UV1Grid[U00], UV1Grid[U10], UV1Grid[U01], UV1Grid[U11],
					ColorGrid[U00], ColorGrid[U10], ColorGrid[U01], ColorGrid[U11], 0);
			}
		}

		// Optional end caps for open torus arcs (ArcAngle < 360)
		if (!bClosedMajor)
		{
			auto AddTorusEndCap = [&](int32 ColIdx, double MajorAngleDeg, bool bFlipWinding, int32 GroupId)
			{
				const double MajorAngle = FMath::DegreesToRadians(MajorAngleDeg);
				const FVector3d CenterPos(MajorR * FMath::Cos(MajorAngle), MajorR * FMath::Sin(MajorAngle), 0.0);
				const float RawU = (ColIdx == 0) ? 0.0f : 1.0f;

				const int32 CenterV = Ctx.Mesh.AppendVertex(CenterPos);
				const int32 CenterUV0 = Ctx.AddUV0(0.5f, 0.5f);
				const int32 CenterUV1 = Ctx.AddUV1(0.0f, 0.5f);
				const int32 CenterCol = Ctx.AddColor(FFXVertexAttr{ RawU, 0.5f, 0.0f });

				for (int32 Row = 0; Row < MinorSegs; ++Row)
				{
					const int32 NextRow = (Row + 1) % MinorSegs;
					const int32 V1 = VertGrid[Row * MajorVerts + ColIdx];
					const int32 V2 = VertGrid[NextRow * MajorVerts + ColIdx];

					const double A1 = (static_cast<double>(Row) / MinorSegs) * UE_DOUBLE_TWO_PI;
					const double A2 = (static_cast<double>(Row + 1) / MinorSegs) * UE_DOUBLE_TWO_PI;
					const int32 U1 = Ctx.AddUV0(0.5f + 0.5f * FMath::Cos(A1), 0.5f + 0.5f * FMath::Sin(A1));
					const int32 U2 = Ctx.AddUV0(0.5f + 0.5f * FMath::Cos(A2), 0.5f + 0.5f * FMath::Sin(A2));
					const int32 U1_1 = Ctx.AddUV1(1.0f, static_cast<float>(Row) / MinorSegs);
					const int32 U1_2 = Ctx.AddUV1(1.0f, static_cast<float>(Row + 1) / MinorSegs);
					const int32 C1 = Ctx.AddColor(FFXVertexAttr{ RawU, static_cast<float>(Row) / MinorSegs, 1.0f });
					const int32 C2 = Ctx.AddColor(FFXVertexAttr{ RawU, static_cast<float>(Row + 1) / MinorSegs, 1.0f });

					if (bFlipWinding)
					{
						Ctx.AddTri(CenterV, V1, V2, CenterUV0, U1, U2, CenterUV1, U1_1, U1_2, CenterCol, C1, C2, GroupId);
					}
					else
					{
						Ctx.AddTri(CenterV, V2, V1, CenterUV0, U2, U1, CenterUV1, U1_2, U1_1, CenterCol, C2, C1, GroupId);
					}
				}
			};

			if (P.bCapStart)
			{
				AddTorusEndCap(0, 0.0, false, 1);
			}
			if (P.bCapEnd)
			{
				AddTorusEndCap(MajorSegs, ArcDeg, true, 2);
			}
		}
	}

	FBox ComputeMeshBounds(const FDynamicMesh3& Mesh)
	{
		FBox Bounds(ForceInit);
		for (int32 Vid : Mesh.VertexIndicesItr())
		{
			Bounds += FVector(Mesh.GetVertex(Vid));
		}
		return Bounds;
	}
}

void FFXMeshBuilder::BuildMesh(
	const FFXMeshGeneratorSettings& Settings,
	const TArray<TObjectPtr<UFXMeshModifier>>& Modifiers,
	FDynamicMesh3& OutMesh)
{
	FMeshBuildContext Ctx(OutMesh, Settings);

	switch (Settings.ShapeType)
	{
	case EFXMeshShapeType::CylinderTube:
	{
		const FFXMeshCylinderParams& P = Settings.Cylinder;
		BuildRevolvedTube(Ctx, P.BottomRadius, P.TopRadius, P.Height, P.ArcAngle,
			P.RadialSegments, P.HeightSegments, P.bCapBottom, P.bCapTop, P.bUseRadiusCurve, P.RadiusProfileCurve);
		break;
	}
	case EFXMeshShapeType::Cone:
	{
		const FFXMeshConeParams& P = Settings.Cone;
		BuildRevolvedTube(Ctx, P.BaseRadius, P.TipRadius, P.Height, P.ArcAngle,
			P.RadialSegments, P.HeightSegments, P.bCapBase, P.bCapTip, P.bUseRadiusCurve, P.RadiusProfileCurve);
		break;
	}
	case EFXMeshShapeType::SlashArc:
		BuildSlashArc(Ctx);
		break;
	case EFXMeshShapeType::DiscRing:
		BuildDiscRing(Ctx);
		break;
	case EFXMeshShapeType::HemisphereDome:
		BuildHemisphereDome(Ctx);
		break;
	case EFXMeshShapeType::Ribbon:
		BuildRibbon(Ctx);
		break;
	case EFXMeshShapeType::CrossPlane:
		BuildCrossPlane(Ctx);
		break;
	case EFXMeshShapeType::Torus:
		BuildTorus(Ctx);
		break;
	default:
	{
		const FFXMeshCylinderParams& P = Settings.Cylinder;
		BuildRevolvedTube(Ctx, P.BottomRadius, P.TopRadius, P.Height, P.ArcAngle,
			P.RadialSegments, P.HeightSegments, P.bCapBottom, P.bCapTop, P.bUseRadiusCurve, P.RadiusProfileCurve);
		break;
	}
	}

	// Apply ordered deformation modifier stack
	for (const TObjectPtr<UFXMeshModifier>& Modifier : Modifiers)
	{
		if (Modifier && Modifier->bEnabled)
		{
			const FBox Bounds = ComputeMeshBounds(OutMesh);
			if (Bounds.IsValid)
			{
				Modifier->Apply(OutMesh, Bounds);
			}
		}
	}

	// Apply Pivot Preset
	const FBox PostBounds = ComputeMeshBounds(OutMesh);
	if (PostBounds.IsValid)
	{
		double ZShift = 0.0;
		switch (Settings.PivotPreset)
		{
		case EFXMeshPivotPreset::Base:
			ZShift = -PostBounds.Min.Z;
			break;
		case EFXMeshPivotPreset::Center:
			ZShift = -PostBounds.GetCenter().Z;
			break;
		case EFXMeshPivotPreset::Top:
			ZShift = -PostBounds.Max.Z;
			break;
		}

		if (!FMath::IsNearlyZero(ZShift))
		{
			for (int32 Vid : OutMesh.VertexIndicesItr())
			{
				FVector3d P = OutMesh.GetVertex(Vid);
				P.Z += ZShift;
				OutMesh.SetVertex(Vid, P);
			}
		}
	}

	// Apply Up-Axis Orientation Alignment
	if (Settings.UpAxis != EFXMeshAxis::Z)
	{
		for (int32 Vid : OutMesh.VertexIndicesItr())
		{
			const FVector3d P = OutMesh.GetVertex(Vid);
			if (Settings.UpAxis == EFXMeshAxis::X)
			{
				OutMesh.SetVertex(Vid, FVector3d(P.Z, P.Y, -P.X));
			}
			else if (Settings.UpAxis == EFXMeshAxis::Y)
			{
				OutMesh.SetVertex(Vid, FVector3d(P.X, P.Z, -P.Y));
			}
		}
	}

	// Recompute smooth normals after all deformations & transforms
	FMeshNormals::InitializeOverlayToPerVertexNormals(OutMesh.Attributes()->PrimaryNormals(), false);
}

void FFXMeshBuilder::ApplyTemplate(
	EFXMeshTemplate Template,
	FFXMeshGeneratorSettings& OutSettings,
	TArray<TObjectPtr<UFXMeshModifier>>& OutModifiers,
	UObject* ModifierOuter)
{
	if (Template == EFXMeshTemplate::Custom)
	{
		return;
	}

	OutSettings = FFXMeshGeneratorSettings();
	OutModifiers.Reset();

	switch (Template)
	{
	case EFXMeshTemplate::SwordSlash:
		OutSettings.ShapeType = EFXMeshShapeType::SlashArc;
		OutSettings.SlashArc.OuterRadius = 120.0f;
		OutSettings.SlashArc.InnerRadius = 60.0f;
		OutSettings.SlashArc.ArcAngle = 160.0f;
		OutSettings.SlashArc.ArchHeight = 12.0f;
		OutSettings.SlashArc.bTaperTips = true;
		OutSettings.SlashArc.ArcSegments = 32;
		OutSettings.SlashArc.CrossSegments = 4;
		OutSettings.PivotPreset = EFXMeshPivotPreset::Center;
		OutSettings.AlphaChannel = EFXMeshVertexColorSource::EdgeFalloffU;
		break;

	case EFXMeshTemplate::GroundShockwave:
		OutSettings.ShapeType = EFXMeshShapeType::DiscRing;
		OutSettings.DiscRing.OuterRadius = 150.0f;
		OutSettings.DiscRing.InnerRadius = 30.0f;
		OutSettings.DiscRing.ArcAngle = 360.0f;
		OutSettings.DiscRing.RadialSegments = 40;
		OutSettings.DiscRing.RingSegments = 6;
		OutSettings.AlphaChannel = EFXMeshVertexColorSource::EdgeFalloffV;
		break;

	case EFXMeshTemplate::EnergyBeam:
		OutSettings.ShapeType = EFXMeshShapeType::CylinderTube;
		OutSettings.Cylinder.BottomRadius = 25.0f;
		OutSettings.Cylinder.TopRadius = 25.0f;
		OutSettings.Cylinder.Height = 300.0f;
		OutSettings.Cylinder.RadialSegments = 24;
		OutSettings.Cylinder.HeightSegments = 16;
		OutSettings.UpAxis = EFXMeshAxis::X;
		OutSettings.AlphaChannel = EFXMeshVertexColorSource::EdgeFalloffV;
		break;

	case EFXMeshTemplate::TornadoVortex:
	{
		OutSettings.ShapeType = EFXMeshShapeType::Cone;
		OutSettings.Cone.BaseRadius = 20.0f;
		OutSettings.Cone.TipRadius = 110.0f;
		OutSettings.Cone.Height = 250.0f;
		OutSettings.Cone.RadialSegments = 32;
		OutSettings.Cone.HeightSegments = 20;
		OutSettings.AlphaChannel = EFXMeshVertexColorSource::EdgeFalloffV;
		if (ModifierOuter)
		{
			UFXMeshTwistModifier* Twist = NewObject<UFXMeshTwistModifier>(ModifierOuter);
			Twist->TwistAngle = 360.0f;
			OutModifiers.Add(Twist);

			UFXMeshWaveModifier* Wave = NewObject<UFXMeshWaveModifier>(ModifierOuter);
			Wave->Amplitude = 12.0f;
			Wave->Frequency = 1.5f;
			OutModifiers.Add(Wave);
		}
		break;
	}

	case EFXMeshTemplate::ShieldDome:
		OutSettings.ShapeType = EFXMeshShapeType::HemisphereDome;
		OutSettings.Dome.Radius = 100.0f;
		OutSettings.Dome.VerticalSweepAngle = 90.0f;
		OutSettings.Dome.HorizontalArcAngle = 360.0f;
		OutSettings.Dome.LongitudeSegments = 36;
		OutSettings.Dome.LatitudeSegments = 14;
		OutSettings.AlphaChannel = EFXMeshVertexColorSource::AxisGradientV;
		break;

	case EFXMeshTemplate::FlameCard:
		OutSettings.ShapeType = EFXMeshShapeType::CrossPlane;
		OutSettings.CrossPlane.PlaneCount = 3;
		OutSettings.CrossPlane.Width = 80.0f;
		OutSettings.CrossPlane.Height = 140.0f;
		OutSettings.CrossPlane.WidthSegments = 4;
		OutSettings.CrossPlane.HeightSegments = 12;
		OutSettings.AlphaChannel = EFXMeshVertexColorSource::EdgeFalloffV;
		if (ModifierOuter)
		{
			UFXMeshTaperModifier* Taper = NewObject<UFXMeshTaperModifier>(ModifierOuter);
			Taper->BottomScale = 1.0f;
			Taper->TopScale = 0.25f;
			OutModifiers.Add(Taper);
		}
		break;

	case EFXMeshTemplate::PortalRing:
		OutSettings.ShapeType = EFXMeshShapeType::Torus;
		OutSettings.Torus.MajorRadius = 100.0f;
		OutSettings.Torus.TubeRadius = 18.0f;
		OutSettings.Torus.ArcAngle = 360.0f;
		OutSettings.Torus.MajorSegments = 40;
		OutSettings.Torus.TubeSegments = 16;
		OutSettings.PivotPreset = EFXMeshPivotPreset::Center;
		break;

	default:
		break;
	}
}
