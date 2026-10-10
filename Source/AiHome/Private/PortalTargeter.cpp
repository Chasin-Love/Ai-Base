#include "PortalTargeter.h"
#include "DrawDebugHelpers.h"
#include "InputCoreTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

APortalTargeter::APortalTargeter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APortalTargeter::BeginPlay()
{
	Super::BeginPlay();
}

void APortalTargeter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	// 1. Where is the player's camera looking?
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	FVector End = CamLoc + CamRot.Vector() * TraceDistance;

	// 2. Shoot an invisible laser there
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (APawn* MyPawn = PC->GetPawn())
	{
		Params.AddIgnoredActor(MyPawn); // don't let the laser hit yourself
	}

	bool bHit = World->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Visibility, Params);

	if (bHit)
	{
		// GREEN dot = where a portal would land (on ANY surface!)
		DrawDebugSphere(World, Hit.Location, 12.f, 12, FColor::Green, false, 0.f);
		// BLUE arrow = the wall's angle (the "surface normal")
		DrawDebugLine(World, Hit.Location, Hit.Location + Hit.ImpactNormal * 50.f, FColor::Blue, false, 0.f);

		// LEFT CLICK = RED flash test (future portal spawn!)
		if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			if (PortalClass)
			{
				FTransform SpawnTransform;
				SpawnTransform.SetLocation(Hit.Location);
				SpawnTransform.SetRotation(FQuat::Identity); // We will fix the wall rotation next!

				GetWorld()->SpawnActor<AActor>(PortalClass, SpawnTransform);
			}
		}
	}
}