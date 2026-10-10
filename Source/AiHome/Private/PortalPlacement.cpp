#include "PortalPlacement.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	constexpr float MovingBodySpeedSq = 25.f; // ~5 cm/s

	bool IsMovingBodyActor(const AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}
		if (Actor->IsA<APawn>())
		{
			return true;
		}
		if (Actor->GetVelocity().SizeSquared() > MovingBodySpeedSq)
		{
			return true;
		}
		return false;
	}
}

bool FPortalPlacement::IsMovingSurface(const FHitResult& Hit)
{
	const UPrimitiveComponent* Primitive = Hit.GetComponent();
	if (Primitive)
	{
		// Static mobility can never be moving
		if (Primitive->Mobility == EComponentMobility::Static)
		{
			return false;
		}
		if (Primitive->IsSimulatingPhysics())
		{
			return true;
		}
		if (Primitive->GetComponentVelocity().SizeSquared() > MovingBodySpeedSq)
		{
			return true;
		}
		if (IsMovingBodyActor(Primitive->GetAttachmentRootActor()))
		{
			return true;
		}
	}

	return IsMovingBodyActor(Hit.GetActor());
}

void FPortalPlacement::BuildPortalAxes(const FVector& Normal, FVector& OutRight, FVector& OutUp)
{
	const bool bFloorOrCeiling = FMath::Abs(FVector::DotProduct(Normal, FVector::UpVector)) > 0.9f;
	const FVector ReferenceUp = bFloorOrCeiling ? FVector::ForwardVector : FVector::UpVector;
	OutRight = FVector::CrossProduct(ReferenceUp, Normal).GetSafeNormal();
	if (OutRight.IsNearlyZero())
	{
		OutRight = FVector::CrossProduct(FVector::RightVector, Normal).GetSafeNormal();
	}
	OutUp = FVector::CrossProduct(Normal, OutRight).GetSafeNormal();
}

const TCHAR* FPortalPlacement::FailLabel(EPortalPlacementFail Reason)
{
	switch (Reason)
	{
	case EPortalPlacementFail::None: return TEXT("LEGAL");
	case EPortalPlacementFail::NoHit: return TEXT("SKY");
	case EPortalPlacementFail::Moving: return TEXT("MOVING");
	case EPortalPlacementFail::NotFlat: return TEXT("NOT FLAT");
	case EPortalPlacementFail::Edge: return TEXT("EDGE");
	default: return TEXT("?");
	}
}

FPortalPlacementResult FPortalPlacement::Evaluate(
	UWorld* World,
	const FVector& TraceStart,
	const FVector& TraceEnd,
	const FCollisionQueryParams& QueryParams,
	ECollisionChannel Channel,
	const FPortalPlacementTuning& Tuning)
{
	FPortalPlacementResult Result;
	if (!World)
	{
		return Result;
	}

	FHitResult CenterHit;
	if (!World->LineTraceSingleByChannel(CenterHit, TraceStart, TraceEnd, Channel, QueryParams))
	{
		Result.FailReason = EPortalPlacementFail::NoHit;
		return Result;
	}

	Result.Location = CenterHit.ImpactPoint;
	Result.Normal = CenterHit.ImpactNormal.GetSafeNormal();
	BuildPortalAxes(Result.Normal, Result.Right, Result.Up);

	if (IsMovingSurface(CenterHit))
	{
		Result.FailReason = EPortalPlacementFail::Moving;
		return Result;
	}

	bool bAnyEdge = false;
	bool bAnyMoving = false;
	bool bAnyNotFlat = false;

	for (int32 Index = 0; Index < FPortalPlacementTuning::SampleCount; ++Index)
	{
		const float Angle = (2.f * PI) * (static_cast<float>(Index) / static_cast<float>(FPortalPlacementTuning::SampleCount));
		FPortalPlacementSample Sample;
		Sample.PlanePoint = Result.Location
			+ Result.Right * (FMath::Cos(Angle) * Tuning.HalfWidth)
			+ Result.Up * (FMath::Sin(Angle) * Tuning.HalfHeight);

		const FVector ProbeStart = Sample.PlanePoint + Result.Normal * Tuning.ProbeFront;
		const FVector ProbeEnd = Sample.PlanePoint - Result.Normal * Tuning.ProbeBack;

		FHitResult SampleHit;
		if (!World->LineTraceSingleByChannel(SampleHit, ProbeStart, ProbeEnd, Channel, QueryParams))
		{
			bAnyEdge = true;
			Result.Samples.Add(Sample);
			continue;
		}

		if (IsMovingSurface(SampleHit))
		{
			bAnyMoving = true;
			Result.Samples.Add(Sample);
			continue;
		}

		const float NormalDot = FVector::DotProduct(SampleHit.ImpactNormal.GetSafeNormal(), Result.Normal);
		const float PlaneDist = FMath::Abs(FVector::DotProduct(SampleHit.ImpactPoint - Result.Location, Result.Normal));
		if (NormalDot < Tuning.NormalDotMin || PlaneDist > Tuning.PlaneTolerance)
		{
			bAnyNotFlat = true;
			Result.Samples.Add(Sample);
			continue;
		}

		Sample.bPassed = true;
		Result.Samples.Add(Sample);
	}

	if (bAnyMoving)
	{
		Result.FailReason = EPortalPlacementFail::Moving;
		return Result;
	}
	if (bAnyEdge)
	{
		Result.FailReason = EPortalPlacementFail::Edge;
		return Result;
	}
	if (bAnyNotFlat)
	{
		Result.FailReason = EPortalPlacementFail::NotFlat;
		return Result;
	}

	Result.bIsLegal = true;
	Result.FailReason = EPortalPlacementFail::None;
	return Result;
}
