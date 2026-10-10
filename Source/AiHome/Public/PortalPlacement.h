#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"

class UWorld;

enum class EPortalPlacementFail : uint8
{
	None,
	NoHit,
	Moving,
	NotFlat,
	Edge
};

struct FPortalPlacementSample
{
	FVector PlanePoint = FVector::ZeroVector;
	bool bPassed = false;
};

struct FPortalPlacementTuning
{
	float HalfWidth = 60.f;
	float HalfHeight = 100.f;
	float NormalDotMin = 0.92f;
	float PlaneTolerance = 12.f;
	float ProbeFront = 25.f;
	float ProbeBack = 40.f;
	static constexpr int32 SampleCount = 8;
};

struct FPortalPlacementResult
{
	bool bIsLegal = false;
	EPortalPlacementFail FailReason = EPortalPlacementFail::NoHit;
	FVector Location = FVector::ZeroVector;
	FVector Normal = FVector::UpVector;
	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;
	TArray<FPortalPlacementSample, TInlineAllocator<FPortalPlacementTuning::SampleCount>> Samples;
};

struct FPortalPlacement
{
	static FPortalPlacementResult Evaluate(
		UWorld* World,
		const FVector& TraceStart,
		const FVector& TraceEnd,
		const FCollisionQueryParams& QueryParams,
		ECollisionChannel Channel,
		const FPortalPlacementTuning& Tuning);

	static const TCHAR* FailLabel(EPortalPlacementFail Reason);

private:
	static bool IsMovingSurface(const FHitResult& Hit);
	static void BuildPortalAxes(const FVector& Normal, FVector& OutRight, FVector& OutUp);
};
