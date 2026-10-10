#include "PortalTargeter.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "InputCoreTypes.h"

bool FPortalTunnel::IsLinked() const
{
	return SlotA.bIsActive && IsValid(SlotA.SpawnedActor) &&
	       SlotB.bIsActive && IsValid(SlotB.SpawnedActor);
}

bool FPortalTunnel::HasAny() const
{
	return (SlotA.bIsActive && IsValid(SlotA.SpawnedActor)) ||
	       (SlotB.bIsActive && IsValid(SlotB.SpawnedActor));
}

double FPortalTunnel::GetOldestPlacedTime() const
{
	if (SlotA.bIsActive && SlotB.bIsActive)
	{
		return FMath::Min(SlotA.PlacedTime, SlotB.PlacedTime);
	}
	if (SlotA.bIsActive)
	{
		return SlotA.PlacedTime;
	}
	if (SlotB.bIsActive)
	{
		return SlotB.PlacedTime;
	}
	return 0.0;
}

APortalTargeter::APortalTargeter()
{
	PrimaryActorTick.bCanEverTick = true;

	Tunnels.SetNum(2);
	Tunnels[0].TunnelId = 1;
	Tunnels[1].TunnelId = 2;
}

void APortalTargeter::BeginPlay()
{
	Super::BeginPlay();

	if (Tunnels.Num() != 2)
	{
		Tunnels.SetNum(2);
	}
	Tunnels[0].TunnelId = 1;
	Tunnels[1].TunnelId = 2;

	// Load BP_Portal fallback if not assigned
	if (!PortalClass)
	{
		PortalClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/Characters/BP_Portal.BP_Portal_C"));
	}
}

void APortalTargeter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector CamForward = CamRot.Vector();

	// 1. Process inputs
	if (PC->WasInputKeyJustPressed(EKeys::Z))
	{
		HandleToggleArmedType();
	}

	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		HandleFire(World, PC, CamLoc, CamForward);
	}

	// 2. Optional continuous preview (Stage 1 style, off by default)
	if (bContinuousDebugPreview)
	{
		const FVector TraceEnd = CamLoc + CamForward * TraceDistance;
		FCollisionQueryParams Params = MakePlacementQueryParams(PC->GetPawn());

		FPortalPlacementTuning Tuning;
		Tuning.HalfWidth = PortalHalfWidth;
		Tuning.HalfHeight = PortalHalfHeight;
		Tuning.NormalDotMin = NormalDotMin;
		Tuning.PlaneTolerance = PlaneTolerance;

		const FPortalPlacementResult PreviewPlacement = FPortalPlacement::Evaluate(
			World,
			CamLoc,
			TraceEnd,
			Params,
			ECC_Visibility,
			Tuning);

		DrawPlacementPreview(World, CamLoc, CamForward, PreviewPlacement);
	}

	// 3. Render HUD and in-world feedback
	RenderHUD();
	RenderActivePortalsInWorld(World);
	RenderFailFeedback(World);
}

FCollisionQueryParams APortalTargeter::MakePlacementQueryParams(const APawn* MyPawn) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PortalAim), true);
	Params.AddIgnoredActor(this);

	if (MyPawn)
	{
		Params.AddIgnoredActor(MyPawn);

		TArray<AActor*> AttachedActors;
		MyPawn->GetAttachedActors(AttachedActors, true, true);
		for (AActor* Attached : AttachedActors)
		{
			Params.AddIgnoredActor(Attached);
		}
	}

	// Never target an existing portal (placements should land on real surfaces)
	for (const FPortalTunnel& Tunnel : Tunnels)
	{
		if (IsValid(Tunnel.SlotA.SpawnedActor))
		{
			Params.AddIgnoredActor(Tunnel.SlotA.SpawnedActor);
		}
		if (IsValid(Tunnel.SlotB.SpawnedActor))
		{
			Params.AddIgnoredActor(Tunnel.SlotB.SpawnedActor);
		}
	}

	return Params;
}

void APortalTargeter::HandleToggleArmedType()
{
	ArmedType = (ArmedType == EPortalType::PortalA) ? EPortalType::PortalB : EPortalType::PortalA;

	const FString TypeStr = (ArmedType == EPortalType::PortalA) ? TEXT("PORTAL A") : TEXT("PORTAL B");
	const FColor MsgColor = (ArmedType == EPortalType::PortalA) ? FColor(0, 195, 255) : FColor(255, 115, 0);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			99,
			2.0f,
			MsgColor,
			FString::Printf(TEXT("Switched Armed Portal: %s"), *TypeStr));
	}
}

void APortalTargeter::HandleFire(
	UWorld* World,
	APlayerController* PC,
	const FVector& CameraLocation,
	const FVector& CameraForward)
{
	const FVector TraceEnd = CameraLocation + CameraForward * TraceDistance;
	FCollisionQueryParams Params = MakePlacementQueryParams(PC->GetPawn());

	FPortalPlacementTuning Tuning;
	Tuning.HalfWidth = PortalHalfWidth;
	Tuning.HalfHeight = PortalHalfHeight;
	Tuning.NormalDotMin = NormalDotMin;
	Tuning.PlaneTolerance = PlaneTolerance;

	const FPortalPlacementResult Placement = FPortalPlacement::Evaluate(
		World,
		CameraLocation,
		TraceEnd,
		Params,
		ECC_Visibility,
		Tuning);

	if (!Placement.bIsLegal)
	{
		// Failed shot: record fail reason, display feedback, draw red laser beam
		LastFailTime = World->GetTimeSeconds();
		LastFailReason = Placement.FailReason;
		LastFailLocation = (Placement.FailReason == EPortalPlacementFail::NoHit)
			? (CameraLocation + CameraForward * 800.f)
			: Placement.Location;

		DrawDebugLine(World, CameraLocation + CameraForward * 30.f, LastFailLocation, FColor::Red, false, 0.35f, 0, 2.5f);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				98,
				2.0f,
				FColor::Red,
				FString::Printf(TEXT("CANNOT PLACE PORTAL: %s"), FPortalPlacement::FailLabel(Placement.FailReason)));
		}
		return;
	}

	// Legal shot: place portal in the resolved tunnel
	const int32 TargetTunnel = ResolveTargetTunnel(ArmedType);
	PlacePortalInSlot(TargetTunnel, ArmedType, Placement);

	// Beam effect in portal color
	const FColor PortalColor = GetPortalColor(Tunnels[TargetTunnel].TunnelId, ArmedType);
	DrawDebugLine(World, CameraLocation + CameraForward * 30.f, Placement.Location, PortalColor, false, 0.35f, 0, 3.0f);

	// Auto-flip ArmedType to opposite for natural A -> B loop
	ArmedType = (ArmedType == EPortalType::PortalA) ? EPortalType::PortalB : EPortalType::PortalA;
}

int32 APortalTargeter::ResolveTargetTunnel(EPortalType TypeToPlace)
{
	const int32 TotalTunnels = Tunnels.Num();

	// 1. Look for a tunnel with the OPPOSITE type already active, waiting for this type to link
	for (int32 Index = 0; Index < TotalTunnels; ++Index)
	{
		const bool bHasOpposite = (TypeToPlace == EPortalType::PortalA) ? Tunnels[Index].SlotB.bIsActive : Tunnels[Index].SlotA.bIsActive;
		const bool bHasSame = (TypeToPlace == EPortalType::PortalA) ? Tunnels[Index].SlotA.bIsActive : Tunnels[Index].SlotB.bIsActive;
		if (bHasOpposite && !bHasSame)
		{
			return Index;
		}
	}

	// 2. Look for a tunnel with an unpaired portal of the SAME type (player adjusting an unlinked door)
	for (int32 Index = 0; Index < TotalTunnels; ++Index)
	{
		const bool bHasOpposite = (TypeToPlace == EPortalType::PortalA) ? Tunnels[Index].SlotB.bIsActive : Tunnels[Index].SlotA.bIsActive;
		const bool bHasSame = (TypeToPlace == EPortalType::PortalA) ? Tunnels[Index].SlotA.bIsActive : Tunnels[Index].SlotB.bIsActive;
		if (bHasSame && !bHasOpposite)
		{
			return Index;
		}
	}

	// 3. Look for an entirely empty tunnel
	for (int32 Index = 0; Index < TotalTunnels; ++Index)
	{
		if (!Tunnels[Index].HasAny())
		{
			return Index;
		}
	}

	// 4. Both tunnels are fully linked! Replace the oldest tunnel (FIFO)
	int32 OldestIndex = 0;
	double OldestTime = Tunnels[0].GetOldestPlacedTime();
	for (int32 Index = 1; Index < TotalTunnels; ++Index)
	{
		const double TunnelTime = Tunnels[Index].GetOldestPlacedTime();
		if (TunnelTime < OldestTime)
		{
			OldestTime = TunnelTime;
			OldestIndex = Index;
		}
	}

	return OldestIndex;
}

void APortalTargeter::PlacePortalInSlot(
	int32 TunnelIndex,
	EPortalType Type,
	const FPortalPlacementResult& Placement)
{
	UWorld* World = GetWorld();
	if (!World || !Tunnels.IsValidIndex(TunnelIndex))
	{
		return;
	}

	FPortalSlot& Slot = (Type == EPortalType::PortalA) ? Tunnels[TunnelIndex].SlotA : Tunnels[TunnelIndex].SlotB;
	const FVector SpawnLocation = Placement.Location + Placement.Normal * 2.0f;
	const FRotator SpawnRotation = FRotationMatrix::MakeFromXZ(Placement.Normal, Placement.Up).Rotator();

	if (IsValid(Slot.SpawnedActor))
	{
		Slot.SpawnedActor->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
	}
	else
	{
		if (PortalClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Slot.SpawnedActor = World->SpawnActor<AActor>(PortalClass, SpawnLocation, SpawnRotation, SpawnParams);
		}
	}

	Slot.Location = SpawnLocation;
	Slot.Rotation = SpawnRotation;
	Slot.Normal = Placement.Normal;
	Slot.Right = Placement.Right;
	Slot.Up = Placement.Up;
	Slot.bIsActive = true;
	Slot.PlacedTime = World->GetTimeSeconds();

	if (IsValid(Slot.SpawnedActor))
	{
		ApplyPortalVisuals(Slot.SpawnedActor, Tunnels[TunnelIndex].TunnelId, Type);
	}

	UpdateTunnelLinking(TunnelIndex);
}

void APortalTargeter::ApplyPortalVisuals(AActor* PortalActor, int32 TunnelId, EPortalType Type)
{
	if (!IsValid(PortalActor))
	{
		return;
	}

	const FColor Color = GetPortalColor(TunnelId, Type);
	const FLinearColor LinearColor(Color);

	// Colorize all mesh components
	TArray<UMeshComponent*> MeshComponents;
	PortalActor->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (Mesh)
		{
			const int32 NumMats = Mesh->GetNumMaterials();
			for (int32 MatIdx = 0; MatIdx < NumMats; ++MatIdx)
			{
				UMaterialInterface* CurrentMat = Mesh->GetMaterial(MatIdx);
				if (CurrentMat)
				{
					UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(CurrentMat);
					if (!DynMat)
					{
						DynMat = UMaterialInstanceDynamic::Create(CurrentMat, Mesh);
						Mesh->SetMaterial(MatIdx, DynMat);
					}
					if (DynMat)
					{
						DynMat->SetVectorParameterValue(FName("Color"), LinearColor);
						DynMat->SetVectorParameterValue(FName("BaseColor"), LinearColor);
						DynMat->SetVectorParameterValue(FName("EmissiveColor"), LinearColor * 3.f);
						DynMat->SetVectorParameterValue(FName("PortalColor"), LinearColor);
					}
				}
			}
		}
	}

	// Add/configure colored glow light
	UPointLightComponent* LightComp = PortalActor->FindComponentByClass<UPointLightComponent>();
	if (!LightComp)
	{
		LightComp = NewObject<UPointLightComponent>(PortalActor, TEXT("PortalGlowLight"));
		if (LightComp)
		{
			LightComp->RegisterComponent();
			LightComp->AttachToComponent(PortalActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			LightComp->SetRelativeLocation(FVector(20.f, 0.f, 0.f));
		}
	}
	if (LightComp)
	{
		LightComp->SetLightColor(LinearColor);
		LightComp->SetIntensity(3000.f);
		LightComp->SetAttenuationRadius(350.f);
	}
}

void APortalTargeter::UpdateTunnelLinking(int32 TunnelIndex)
{
	const FPortalTunnel& Tunnel = Tunnels[TunnelIndex];
	const FString TunnelStr = FString::Printf(TEXT("Tunnel %d"), Tunnel.TunnelId);

	if (Tunnel.IsLinked())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				90 + TunnelIndex,
				3.0f,
				FColor::Green,
				FString::Printf(TEXT("[%s LINKED!] Portal A <=====> Portal B"), *TunnelStr));
		}
	}
	else
	{
		const FString PlacedType = (Tunnel.SlotA.bIsActive) ? TEXT("Portal A") : TEXT("Portal B");
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				90 + TunnelIndex,
				3.0f,
				FColor::Yellow,
				FString::Printf(TEXT("[%s] Placed %s. Waiting for Partner..."), *TunnelStr, *PlacedType));
		}
	}
}

FColor APortalTargeter::GetPortalColor(int32 TunnelId, EPortalType Type)
{
	if (TunnelId == 1)
	{
		// Tunnel 1: Cyan (A) & Vibrant Orange (B)
		return (Type == EPortalType::PortalA) ? FColor(0, 195, 255) : FColor(255, 115, 0);
	}
	else
	{
		// Tunnel 2 (Backup): Neon Violet (A) & Golden Yellow (B)
		return (Type == EPortalType::PortalA) ? FColor(190, 0, 255) : FColor(255, 208, 0);
	}
}

void APortalTargeter::RenderHUD()
{
	if (!GEngine)
	{
		return;
	}

	// Line 1: Gun Armed Status & Controls
	const FString ArmedStr = (ArmedType == EPortalType::PortalA) ? TEXT("PORTAL A") : TEXT("PORTAL B");
	const FColor ArmedColor = (ArmedType == EPortalType::PortalA) ? FColor(0, 215, 255) : FColor(255, 130, 0);

	GEngine->AddOnScreenDebugMessage(
		10,
		0.0f,
		ArmedColor,
		FString::Printf(TEXT("[PORTAL GUN] Armed: %s | [LMB] Fire | [Z] Switch A/B"), *ArmedStr));

	// Line 2: Tunnel 1 Status
	if (Tunnels.IsValidIndex(0))
	{
		const FPortalTunnel& T1 = Tunnels[0];
		FString T1Status = TEXT("[EMPTY]");
		FColor T1Color = FColor::Silver;
		if (T1.IsLinked())
		{
			T1Status = TEXT("[A1: OPEN] <=====> [B1: OPEN]  (LINKED)");
			T1Color = FColor::Green;
		}
		else if (T1.SlotA.bIsActive)
		{
			T1Status = TEXT("[A1: OPEN]  (Waiting for B1)");
			T1Color = FColor(0, 195, 255);
		}
		else if (T1.SlotB.bIsActive)
		{
			T1Status = TEXT("[B1: OPEN]  (Waiting for A1)");
			T1Color = FColor(255, 115, 0);
		}
		GEngine->AddOnScreenDebugMessage(11, 0.0f, T1Color, FString::Printf(TEXT("Tunnel 1: %s"), *T1Status));
	}

	// Line 3: Tunnel 2 Status
	if (Tunnels.IsValidIndex(1))
	{
		const FPortalTunnel& T2 = Tunnels[1];
		FString T2Status = TEXT("[EMPTY]");
		FColor T2Color = FColor::Silver;
		if (T2.IsLinked())
		{
			T2Status = TEXT("[A2: OPEN] <=====> [B2: OPEN]  (LINKED)");
			T2Color = FColor::Green;
		}
		else if (T2.SlotA.bIsActive)
		{
			T2Status = TEXT("[A2: OPEN]  (Waiting for B2)");
			T2Color = FColor(190, 0, 255);
		}
		else if (T2.SlotB.bIsActive)
		{
			T2Status = TEXT("[B2: OPEN]  (Waiting for A2)");
			T2Color = FColor(255, 208, 0);
		}
		GEngine->AddOnScreenDebugMessage(12, 0.0f, T2Color, FString::Printf(TEXT("Tunnel 2: %s"), *T2Status));
	}
}

void APortalTargeter::RenderActivePortalsInWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}

	auto DrawSlotRing = [this, World](const FPortalSlot& Slot, int32 TunnelId, EPortalType Type, bool bIsTunnelLinked)
	{
		if (!Slot.bIsActive || !IsValid(Slot.SpawnedActor))
		{
			return;
		}

		const FColor Color = GetPortalColor(TunnelId, Type);
		const FString TypeName = (Type == EPortalType::PortalA) ? TEXT("Portal A") : TEXT("Portal B");
		const FString LinkStatus = bIsTunnelLinked ? TEXT("LINKED") : TEXT("WAITING");

		// In-world label floating above the portal
		const FVector LabelPos = Slot.Location + Slot.Up * (PortalHalfHeight + 15.f);
		DrawDebugString(
			World,
			LabelPos,
			FString::Printf(TEXT("T%d: %s [%s]"), TunnelId, *TypeName, *LinkStatus),
			nullptr,
			Color,
			0.0f,
			true);

		// Oval perimeter ring in portal color
		constexpr int32 RingPoints = 12;
		FVector PrevPoint = FVector::ZeroVector;
		for (int32 Index = 0; Index <= RingPoints; ++Index)
		{
			const float Angle = (2.f * PI) * (static_cast<float>(Index % RingPoints) / static_cast<float>(RingPoints));
			const FVector Pt = Slot.Location
				+ Slot.Right * (FMath::Cos(Angle) * PortalHalfWidth)
				+ Slot.Up * (FMath::Sin(Angle) * PortalHalfHeight);

			if (Index > 0)
			{
				DrawDebugLine(World, PrevPoint, Pt, Color, false, 0.0f, 0, 2.5f);
			}
			PrevPoint = Pt;
		}
	};

	for (int32 Index = 0; Index < Tunnels.Num(); ++Index)
	{
		const bool bLinked = Tunnels[Index].IsLinked();
		DrawSlotRing(Tunnels[Index].SlotA, Tunnels[Index].TunnelId, EPortalType::PortalA, bLinked);
		DrawSlotRing(Tunnels[Index].SlotB, Tunnels[Index].TunnelId, EPortalType::PortalB, bLinked);
	}
}

void APortalTargeter::RenderFailFeedback(UWorld* World)
{
	if (!World)
	{
		return;
	}

	const float TimeSinceFail = World->GetTimeSeconds() - LastFailTime;
	if (TimeSinceFail < 1.5f && LastFailReason != EPortalPlacementFail::None)
	{
		const FString FailText = FString::Printf(TEXT("FAIL: %s"), FPortalPlacement::FailLabel(LastFailReason));
		DrawDebugString(
			World,
			LastFailLocation + FVector(0.f, 0.f, 20.f),
			*FailText,
			nullptr,
			FColor::Red,
			0.0f,
			true);

		DrawDebugSphere(World, LastFailLocation, 16.f, 12, FColor::Red, false, 0.0f, 0, 2.f);
	}
}

void APortalTargeter::DrawPlacementPreview(
	UWorld* World,
	const FVector& CameraLocation,
	const FVector& CameraForward,
	const FPortalPlacementResult& Placement) const
{
	const FColor Color = Placement.bIsLegal ? FColor::Green : FColor::Red;
	const FVector MarkerLocation = Placement.FailReason == EPortalPlacementFail::NoHit
		? CameraLocation + CameraForward * 200.f
		: Placement.Location;

	DrawDebugSphere(World, MarkerLocation, 12.f, 12, Color, false, 0.f);
	DrawDebugString(
		World,
		MarkerLocation + FVector(0.f, 0.f, 18.f),
		FPortalPlacement::FailLabel(Placement.FailReason),
		nullptr,
		Color,
		0.f,
		true);

	if (Placement.FailReason == EPortalPlacementFail::NoHit)
	{
		return;
	}

	DrawDebugLine(World, Placement.Location, Placement.Location + Placement.Normal * 50.f, Color, false, 0.f);

	if (Placement.Samples.Num() > 0)
	{
		for (int32 Index = 0; Index < Placement.Samples.Num(); ++Index)
		{
			const FPortalPlacementSample& Sample = Placement.Samples[Index];
			const FColor SampleColor = Sample.bPassed ? FColor::Green : FColor::Red;
			DrawDebugSphere(World, Sample.PlanePoint, 6.f, 8, SampleColor, false, 0.f);

			const int32 NextIndex = (Index + 1) % Placement.Samples.Num();
			DrawDebugLine(World, Sample.PlanePoint, Placement.Samples[NextIndex].PlanePoint, Color, false, 0.f);
		}
	}
}
