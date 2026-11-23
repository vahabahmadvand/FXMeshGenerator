// Copyright Epic Games, Inc. All Rights Reserved.

#include "FXMeshGeneratorTypes.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FXMeshGeneratorTypes)

using namespace UE::Geometry;

namespace
{
	double GetAxisCoord(const FVector3d& V, EFXMeshAxis Axis)
	{
		switch (Axis)
		{
		case EFXMeshAxis::X: return V.X;
		case EFXMeshAxis::Y: return V.Y;
		case EFXMeshAxis::Z:
		default: return V.Z;
		}
	}

	void SetAxisCoord(FVector3d& V, EFXMeshAxis Axis, double Val)
	{
		switch (Axis)
		{
		case EFXMeshAxis::X: V.X = Val; break;
		case EFXMeshAxis::Y: V.Y = Val; break;
		case EFXMeshAxis::Z:
		default: V.Z = Val; break;
		}
	}

	double GetNormalizedAxis(const FVector3d& V, const FBox& Bounds, EFXMeshAxis Axis)
	{
		const double MinVal = GetAxisCoord(FVector3d(Bounds.Min), Axis);
		const double MaxVal = GetAxisCoord(FVector3d(Bounds.Max), Axis);
		const double Span = MaxVal - MinVal;
		if (Span <= KINDA_SMALL_NUMBER)
		{
			return 0.5;
		}
		return FMath::Clamp((GetAxisCoord(V, Axis) - MinVal) / Span, 0.0, 1.0);
	}
}

float UFXMeshModifier::EvaluateFalloff(float T) const
{
	if (bUseFalloffCurve && FalloffCurve.GetRichCurveConst() && FalloffCurve.GetRichCurveConst()->GetNumKeys() > 0)
	{
		return FalloffCurve.GetRichCurveConst()->Eval(FMath::Clamp(T, 0.0f, 1.0f));
	}
	return 1.0f;
}

void UFXMeshTaperModifier::Apply(FDynamicMesh3& Mesh, const FBox& Bounds) const
{
	const FVector3d Center(Bounds.GetCenter());

	for (int32 Vid : Mesh.VertexIndicesItr())
	{
		FVector3d P = Mesh.GetVertex(Vid);
		const double T = GetNormalizedAxis(P, Bounds, Axis);
		const double Falloff = static_cast<double>(EvaluateFalloff(static_cast<float>(T)));
		const double Scale = FMath::Lerp(1.0, FMath::Lerp(static_cast<double>(BottomScale), static_cast<double>(TopScale), T), Falloff);

		if (Axis != EFXMeshAxis::X) { P.X = Center.X + (P.X - Center.X) * Scale; }
		if (Axis != EFXMeshAxis::Y) { P.Y = Center.Y + (P.Y - Center.Y) * Scale; }
		if (Axis != EFXMeshAxis::Z) { P.Z = Center.Z + (P.Z - Center.Z) * Scale; }

		Mesh.SetVertex(Vid, P);
	}
}

void UFXMeshTwistModifier::Apply(FDynamicMesh3& Mesh, const FBox& Bounds) const
{
	const FVector3d Center(Bounds.GetCenter());

	for (int32 Vid : Mesh.VertexIndicesItr())
	{
		FVector3d P = Mesh.GetVertex(Vid);
		const double T = GetNormalizedAxis(P, Bounds, Axis);
		const double Falloff = static_cast<double>(EvaluateFalloff(static_cast<float>(T)));
		const double AngleRad = FMath::DegreesToRadians(static_cast<double>(TwistAngle) * T * Falloff);
		const double CosA = FMath::Cos(AngleRad);
		const double SinA = FMath::Sin(AngleRad);

		FVector3d Rel = P - Center;
		FVector3d Rotated = Rel;
		switch (Axis)
		{
		case EFXMeshAxis::X:
			Rotated.Y = Rel.Y * CosA - Rel.Z * SinA;
			Rotated.Z = Rel.Y * SinA + Rel.Z * CosA;
			break;
		case EFXMeshAxis::Y:
			Rotated.X = Rel.X * CosA + Rel.Z * SinA;
			Rotated.Z = -Rel.X * SinA + Rel.Z * CosA;
			break;
		case EFXMeshAxis::Z:
		default:
			Rotated.X = Rel.X * CosA - Rel.Y * SinA;
			Rotated.Y = Rel.X * SinA + Rel.Y * CosA;
			break;
		}

		Mesh.SetVertex(Vid, Center + Rotated);
	}
}

void UFXMeshBendModifier::Apply(FDynamicMesh3& Mesh, const FBox& Bounds) const
{
	if (FMath::IsNearlyZero(BendAngle) || PrimaryAxis == BendDirection)
	{
		return;
	}

	const double MinVal = GetAxisCoord(FVector3d(Bounds.Min), PrimaryAxis);
	const double MaxVal = GetAxisCoord(FVector3d(Bounds.Max), PrimaryAxis);
	const double Length = MaxVal - MinVal;
	if (Length <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const double TotalAngleRad = FMath::DegreesToRadians(static_cast<double>(BendAngle));
	const double BendRadius = Length / TotalAngleRad;

	for (int32 Vid : Mesh.VertexIndicesItr())
	{
		FVector3d P = Mesh.GetVertex(Vid);
		const double T = FMath::Clamp((GetAxisCoord(P, PrimaryAxis) - MinVal) / Length, 0.0, 1.0);
		const double Falloff = static_cast<double>(EvaluateFalloff(static_cast<float>(T)));
		const double Theta = TotalAngleRad * T * Falloff;

		const double BendOffset = GetAxisCoord(P, BendDirection);
		const double R = BendRadius - BendOffset;
		const double NewPrimary = MinVal + R * FMath::Sin(Theta);
		const double NewBend = BendRadius - R * FMath::Cos(Theta);

		SetAxisCoord(P, PrimaryAxis, NewPrimary);
		SetAxisCoord(P, BendDirection, NewBend);
		Mesh.SetVertex(Vid, P);
	}
}

void UFXMeshWaveModifier::Apply(FDynamicMesh3& Mesh, const FBox& Bounds) const
{
	if (FMath::IsNearlyZero(Amplitude))
	{
		return;
	}

	for (int32 Vid : Mesh.VertexIndicesItr())
	{
		FVector3d P = Mesh.GetVertex(Vid);
		const double T = GetNormalizedAxis(P, Bounds, WaveAxis);
		const double Falloff = static_cast<double>(EvaluateFalloff(static_cast<float>(T)));
		const double Offset = static_cast<double>(Amplitude) * Falloff *
			FMath::Sin(T * static_cast<double>(Frequency) * UE_DOUBLE_TWO_PI + static_cast<double>(Phase));

		SetAxisCoord(P, DisplaceAxis, GetAxisCoord(P, DisplaceAxis) + Offset);
		Mesh.SetVertex(Vid, P);
	}
}

void UFXMeshNoiseModifier::Apply(FDynamicMesh3& Mesh, const FBox& Bounds) const
{
	if (FMath::IsNearlyZero(Amplitude))
	{
		return;
	}

	FMeshNormals Normals(&Mesh);
	if (bAlongNormals)
	{
		Normals.ComputeVertexNormals();
	}

	const FVector SeedOffset(
		static_cast<float>(Seed) * 17.13f,
		static_cast<float>(Seed) * 31.71f,
		static_cast<float>(Seed) * 53.37f);

	for (int32 Vid : Mesh.VertexIndicesItr())
	{
		FVector3d P = Mesh.GetVertex(Vid);
		const double T = GetNormalizedAxis(P, Bounds, EFXMeshAxis::Z);
		const double Falloff = static_cast<double>(EvaluateFalloff(static_cast<float>(T)));

		const FVector SamplePos = FVector(P) * Frequency + SeedOffset;
		if (bAlongNormals)
		{
			const double N = static_cast<double>(FMath::PerlinNoise3D(SamplePos));
			P += Normals[Vid] * (N * static_cast<double>(Amplitude) * Falloff);
		}
		else
		{
			const double Nx = static_cast<double>(FMath::PerlinNoise3D(SamplePos));
			const double Ny = static_cast<double>(FMath::PerlinNoise3D(SamplePos + FVector(19.3f, 7.1f, 3.7f)));
			const double Nz = static_cast<double>(FMath::PerlinNoise3D(SamplePos + FVector(41.9f, 23.5f, 11.2f)));
			P += FVector3d(Nx, Ny, Nz) * (static_cast<double>(Amplitude) * Falloff);
		}

		Mesh.SetVertex(Vid, P);
	}
}

void UFXMeshInflateModifier::Apply(FDynamicMesh3& Mesh, const FBox& Bounds) const
{
	if (FMath::IsNearlyZero(Amount))
	{
		return;
	}

	FMeshNormals Normals(&Mesh);
	Normals.ComputeVertexNormals();

	for (int32 Vid : Mesh.VertexIndicesItr())
	{
		FVector3d P = Mesh.GetVertex(Vid);
		const double T = GetNormalizedAxis(P, Bounds, EFXMeshAxis::Z);
		double Falloff = static_cast<double>(EvaluateFalloff(static_cast<float>(T)));
		if (bMidBulgeFalloff)
		{
			Falloff *= FMath::Sin(UE_DOUBLE_PI * T);
		}

		P += Normals[Vid] * (static_cast<double>(Amount) * Falloff);
		Mesh.SetVertex(Vid, P);
	}
}
