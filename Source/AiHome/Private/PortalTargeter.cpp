#include "PortalTargeter.h"
#include "AiHomePortalHUD.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
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

	// Stage 7: keep the render target / capture slots allocated before any portal is placed
	PortalRenderTargets.SetNum(2);
	PortalCaptureComponents.SetNum(2);
	PortalRenderTargets[0] = nullptr;
	PortalRenderTargets[1] = nullptr;
	PortalCaptureComponents[0] = nullptr;
	PortalCaptureComponents[1] = nullptr;
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

	// Loud warning if the portal visual class is missing - otherwise portals would place silently
	if (!PortalClass && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			10.f,
			FColor::Red,
			TEXT("PortalTargeter: BP_Portal class not found - portals cannot spawn. Check /Game/Characters/BP_Portal."));
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

	if (bDebugSpawnBallOnB && PC->WasInputKeyJustPressed(EKeys::B))
	{
		HandleDebugSpawnBall(World, CamLoc, CamForward);
	}

	// 2. Stage 3+4: the player, props and other characters travel through linked portals
	UpdatePortalTeleport(World, PC);

	// 2b. Stage 5: install the crosshair HUD once, and pulse portal glow by linking state
	if (bCrosshairEnabled && !bHudInitialized)
	{
		AHUD* CurrentHUD = PC->GetHUD();
		if (!CurrentHUD || !CurrentHUD->IsA<AAiHomePortalHUD>())
		{
			PC->ClientSetHUD(AAiHomePortalHUD::StaticClass());
		}
		bHudInitialized = true;
	}

	// Stage 7: ensure render target array and capture array stay sized with active portals	

	UpdatePortalVisuals(World, DeltaTime);

	// Stage 7: update (or hide) each portal's live view after the visuals pulse this frame
	UpdatePortalCaptures(World);

	// 3. Optional continuous preview (Stage 1 style, off by default)
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

	// 4. Stage 2 test overlay (off for clean gameplay; enable on the actor for diagnostics)
	if (bShowTestOverlay)
	{
		RenderHUD();
		RenderActivePortalsInWorld(World);
	}

	// 5. An illegal shot always shows a short reason at the impact point
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

void APortalTargeter::UpdatePortalTeleport(UWorld* World, APlayerController* PC)
{
	if (!bTeleportEnabled || !World || !PC)
	{
		return;
	}

	APawn* PlayerPawn = PC->GetPawn();

	// The player goes first: needs the controller view and rotation
	if (PlayerPawn && World->GetTimeSeconds() >= TeleportCooldownUntil)
	{
		for (int32 TunnelIndex = 0; TunnelIndex < Tunnels.Num(); ++TunnelIndex)
		{
			const FPortalTunnel& Tunnel = Tunnels[TunnelIndex];
			if (!Tunnel.IsLinked())
			{
				continue;
			}

			if (TryTeleportThroughPortal(Tunnel.SlotA, Tunnel.SlotB, Tunnel.TunnelId, PC, PlayerPawn) ||
				TryTeleportThroughPortal(Tunnel.SlotB, Tunnel.SlotA, Tunnel.TunnelId, PC, PlayerPawn))
			{
				break;
			}
		}
	}

	// Then physics props and other characters
	if (!bObjectsTeleportEnabled)
	{
		return;
	}

	for (int32 TunnelIndex = 0; TunnelIndex < Tunnels.Num(); ++TunnelIndex)
	{
		const FPortalTunnel& Tunnel = Tunnels[TunnelIndex];
		if (!Tunnel.IsLinked())
		{
			continue;
		}

		TryTeleportObjectsThroughPortal(World, Tunnel.SlotA, Tunnel.SlotB, Tunnel.TunnelId, PlayerPawn);
		TryTeleportObjectsThroughPortal(World, Tunnel.SlotB, Tunnel.SlotA, Tunnel.TunnelId, PlayerPawn);
	}
}

bool APortalTargeter::TryTeleportThroughPortal(
	const FPortalSlot& Entry,
	const FPortalSlot& Exit,
	int32 TunnelId,
	APlayerController* PC,
	APawn* Pawn)
{
	if (!Entry.bIsActive || !Exit.bIsActive ||
		!IsValid(Entry.SpawnedActor) || !IsValid(Exit.SpawnedActor))
	{
		return false;
	}

	const FVector PawnLocation = Pawn->GetActorLocation();
	const FVector PawnVelocity = Pawn->GetVelocity();

	// How far the pawn's center is from the portal plane (positive = in front of the portal)
	const FVector ToPawn = PawnLocation - Entry.Location;
	const float DistanceToPlane = FVector::DotProduct(ToPawn, Entry.Normal);

	// Capsule-aware: the surface of the player's capsule must reach (or pass) the portal plane.
	// This makes both wall portals (side of the capsule) and floor portals (bottom of the capsule) work.
	float CapsuleExtent = 0.f;
	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			const float AxisAlign = FMath::Abs(FVector::DotProduct(Capsule->GetUpVector(), Entry.Normal));
			CapsuleExtent = Capsule->GetScaledCapsuleRadius() * FMath::Sqrt(FMath::Max(0.f, 1.f - AxisAlign * AxisAlign))
				+ Capsule->GetScaledCapsuleHalfHeight() * AxisAlign;
		}
	}

	const float SurfaceGap = DistanceToPlane - CapsuleExtent;
	if (SurfaceGap > TeleportTriggerDistance)
	{
		return false;
	}

	// The pawn's center must be inside the portal oval
	const float LocalRight = FVector::DotProduct(ToPawn, Entry.Right);
	const float LocalUp = FVector::DotProduct(ToPawn, Entry.Up);
	constexpr float FitMargin = 0.9f;
	if (FMath::Abs(LocalRight) > PortalHalfWidth * FitMargin ||
		FMath::Abs(LocalUp) > PortalHalfHeight * FitMargin)
	{
		return false;
	}

	// The pawn must actually be heading into the portal: walking input or falling velocity
	const float VelocityIntoPortal = -FVector::DotProduct(PawnVelocity, Entry.Normal);
	const float InputIntoPortal = -FVector::DotProduct(Pawn->GetLastMovementInputVector(), Entry.Normal);
	if (VelocityIntoPortal < 40.f && InputIntoPortal < 0.35f)
	{
		return false;
	}

	// Map the entry portal's frame onto the exit portal's frame.
	// The 180-degree turn about the portal's local up axis is what sends motion INTO the entry
	// out of the exit (velocity, view and strafing stay consistent with a real portal pair).
	const FQuat EntryQuat = FRotationMatrix::MakeFromXZ(Entry.Normal, Entry.Up).ToQuat();
	const FQuat ExitQuat = FRotationMatrix::MakeFromXZ(Exit.Normal, Exit.Up).ToQuat();
	const FQuat PortalDelta = ExitQuat * FQuat(FVector::UpVector, PI) * EntryQuat.Inverse();

	const FVector TangentialOffset = Entry.Right * LocalRight + Entry.Up * LocalUp;
	FVector ExitLocation = Exit.Location
		+ PortalDelta.RotateVector(TangentialOffset)
		+ Exit.Normal * TeleportExitClearance;

	// Never drop the player inside geometry: nudge further out of the exit when the spot is blocked
	ExitLocation = FindSafeExitLocation(GetWorld(), ExitLocation, Exit.Normal, 42.f);

	const FVector ExitVelocity = PortalDelta.RotateVector(PawnVelocity);

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ExitViewDirection = PortalDelta.RotateVector(ViewRotation.Vector());
	const FRotator ExitControlRotation = ExitViewDirection.Rotation();

	// Move the player through
	const FRotator ExitActorRotation(0.f, ExitControlRotation.Yaw, 0.f);
	Pawn->TeleportTo(ExitLocation, ExitActorRotation, false, true);

	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->Velocity = ExitVelocity;
		}
	}

	if (USceneComponent* Root = Pawn->GetRootComponent())
	{
		Root->ComponentVelocity = ExitVelocity;
	}

	PC->SetControlRotation(ExitControlRotation);

	if (UWorld* World = GetWorld())
	{
		TeleportCooldownUntil = World->GetTimeSeconds() + TeleportCooldown;
	}

	// Stage 5: a short screen flash sells the transit
	if (bTeleportFeedbackEnabled && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.65f, 0.f, 0.3f, FLinearColor(0.25f, 0.8f, 1.f, 1.f), false, false);
	}

	UE_LOG(LogTemp, Log, TEXT("Portal teleport: tunnel %d, pawn %s"), TunnelId, *GetNameSafe(Pawn));

	if (bShowTestOverlay && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			94,
			2.0f,
			FColor::Green,
			FString::Printf(TEXT("[Tunnel %d] Teleported"), TunnelId));
	}

	return true;
}

FVector APortalTargeter::FindSafeExitLocation(UWorld* World, const FVector& DesiredLocation, const FVector& ExitNormal, float ProbeRadius) const
{
	if (!World)
	{
		return DesiredLocation;
	}

	auto IsBlocked = [World, ProbeRadius](const FVector& Location)
	{
		return World->OverlapBlockingTestByChannel(
			Location,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeSphere(ProbeRadius));
	};

	if (!IsBlocked(DesiredLocation))
	{
		return DesiredLocation;
	}

	// Push a little further out of the exit portal until the player fits
	for (float Push = 40.f; Push <= 200.f; Push += 40.f)
	{
		const FVector Candidate = DesiredLocation + ExitNormal * Push;
		if (!IsBlocked(Candidate))
		{
			return Candidate;
		}
	}

	return DesiredLocation;
}

void APortalTargeter::UpdatePortalVisuals(UWorld* World, float DeltaTime)
{
	PortalVisualTime += DeltaTime;
	PortalShimmerTime += DeltaTime;

	for (const FPortalTunnel& Tunnel : Tunnels)
	{
		const bool bLinked = Tunnel.IsLinked();
		const float Pulse = FMath::Sin(PortalVisualTime * (bLinked ? 2.5f : 6.0f));

		const FPortalSlot* Slots[2] = { &Tunnel.SlotA, &Tunnel.SlotB };

		// Give the two portals in a pair a slightly different shimmer so they don't look identical
		float SlotShimmerPhase[2] = { 0.f, 0.f };
		SlotShimmerPhase[0] = PortalShimmerTime * ShimmerSpeed;
		SlotShimmerPhase[1] = PortalShimmerTime * ShimmerSpeed + ShimmerPhaseOffset;

		for (int32 SlotIndex = 0; SlotIndex < 2; ++SlotIndex)
		{
			const FPortalSlot* Slot = Slots[SlotIndex];
			if (!Slot->bIsActive || !IsValid(Slot->SpawnedActor))
			{
				continue;
			}

			// Light glow
			UPointLightComponent* Glow = Slot->SpawnedActor->FindComponentByClass<UPointLightComponent>();
			if (Glow)
			{
				const float BaseIntensity = bLinked ? 2600.f : 1600.f;
				Glow->SetIntensity(BaseIntensity + Pulse * (bLinked ? 350.f : 900.f));
			}

			// Material-driven ring + shimmer + glow
			SetPortalMaterialParams(
				Slot->SpawnedActor,
				GetPortalColor(Tunnel.TunnelId, SlotIndex == 0 ? EPortalType::PortalA : EPortalType::PortalB),
				SlotShimmerPhase[SlotIndex],
				bLinked,
				Pulse);
		}
	}
}

void APortalTargeter::SetPortalMaterialParams(
	AActor* PortalActor,
	const FColor& Color,
	float ShimmerPhase,
	bool bIsLinked,
	float PortalPulse)
{
	if (!IsValid(PortalActor))
	{
		return;
	}

	const FLinearColor LinearColor(Color);
	const float BaseEmissiveIntensity = bIsLinked ? GlowEmissiveIntensity : (GlowEmissiveIntensity * 0.6f);

	TArray<UMeshComponent*> MeshComponents;
	PortalActor->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (!Mesh)
		{
			continue;
		}

		const int32 NumMats = Mesh->GetNumMaterials();
		for (int32 MatIdx = 0; MatIdx < NumMats; ++MatIdx)
		{
			UMaterialInterface* CurrentMat = Mesh->GetMaterial(MatIdx);
			if (!CurrentMat)
			{
				continue;
			}

			UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(CurrentMat);
			if (!DynMat)
			{
				DynMat = UMaterialInstanceDynamic::Create(CurrentMat, Mesh);
				Mesh->SetMaterial(MatIdx, DynMat);
			}
			if (!DynMat)
			{
				continue;
			}

			DynMat->SetVectorParameterValue(FName("Color"), LinearColor);
			DynMat->SetVectorParameterValue(FName("BaseColor"), LinearColor);
			DynMat->SetVectorParameterValue(FName("EmissiveColor"), LinearColor * BaseEmissiveIntensity);
			DynMat->SetVectorParameterValue(FName("PortalColor"), LinearColor);

			// Stage 6: ring + shimmer + glow (graceful no-ops if the material doesn't have these params)
			DynMat->SetScalarParameterValue(FName("RenderRing"), bRenderRing ? 1.f : 0.f);
			DynMat->SetScalarParameterValue(FName("RingThickness"), RingThickness);
			DynMat->SetScalarParameterValue(FName("ShimmerSpeed"), ShimmerSpeed);
			DynMat->SetScalarParameterValue(FName("ShimmerPhase"), ShimmerPhase);
			DynMat->SetScalarParameterValue(FName("PortalPulse"), PortalPulse);
		}
	}
}

void APortalTargeter::TryTeleportObjectsThroughPortal(
	UWorld* World,
	const FPortalSlot& Entry,
	const FPortalSlot& Exit,
	int32 TunnelId,
	const APawn* PlayerPawn)
{
	if (!Entry.bIsActive || !Exit.bIsActive || !IsValid(Entry.SpawnedActor) || !IsValid(Exit.SpawnedActor))
	{
		return;
	}

	// Find loose things hanging around the portal mouth: physics props and other characters
	const float QueryRadius = FMath::Max(PortalHalfWidth, PortalHalfHeight) * 1.5f + 100.f;
	const FVector QueryCenter = Entry.Location + Entry.Normal * 50.f;

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	if (!World->OverlapMultiByObjectType(
		Overlaps,
		QueryCenter,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(QueryRadius)))
	{
		return;
	}

	TSet<AActor*> HandledActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* Primitive = Overlap.GetComponent();
		AActor* Actor = Overlap.GetActor();
		if (!Primitive || !Actor || Actor == this || Actor == PlayerPawn)
		{
			continue;
		}
		if (Actor == Entry.SpawnedActor || Actor == Exit.SpawnedActor)
		{
			continue;
		}
		if (HandledActors.Contains(Actor))
		{
			continue;
		}
		HandledActors.Add(Actor);

		// Only physics bodies and pawns can travel through; world geometry cannot
		if (!Primitive->IsSimulatingPhysics() && !Actor->IsA<APawn>())
		{
			continue;
		}

		if (const double* CooldownEnd = ObjectTeleportCooldowns.Find(Actor))
		{
			if (*CooldownEnd > World->GetTimeSeconds())
			{
				continue;
			}
		}

		TryTeleportObjectThroughPortal(World, Entry, Exit, TunnelId, Primitive);
	}
}

bool APortalTargeter::TryTeleportObjectThroughPortal(
	UWorld* World,
	const FPortalSlot& Entry,
	const FPortalSlot& Exit,
	int32 TunnelId,
	UPrimitiveComponent* Primitive)
{
	AActor* Actor = Primitive ? Primitive->GetOwner() : nullptr;
	if (!World || !Actor)
	{
		return false;
	}

	const FVector ObjectLocation = Primitive->GetComponentLocation();
	const FVector ObjectVelocity = Primitive->GetComponentVelocity();

	const FVector ToObject = ObjectLocation - Entry.Location;
	const float DistanceToPlane = FVector::DotProduct(ToObject, Entry.Normal);

	// How far the object reaches toward the portal along the normal (box support distance)
	const FBox ObjectBox = Actor->GetComponentsBoundingBox(true);
	const FVector BoxExtent = ObjectBox.GetExtent();
	const float ObjectExtent = FMath::Abs(BoxExtent.X * Entry.Normal.X)
		+ FMath::Abs(BoxExtent.Y * Entry.Normal.Y)
		+ FMath::Abs(BoxExtent.Z * Entry.Normal.Z);

	// The object's surface must reach the portal plane...
	const float SurfaceGap = DistanceToPlane - ObjectExtent;
	if (SurfaceGap > TeleportTriggerDistance || DistanceToPlane < -(ObjectExtent + 25.f))
	{
		return false;
	}

	// ...its center must be inside the oval...
	const float LocalRight = FVector::DotProduct(ToObject, Entry.Right);
	const float LocalUp = FVector::DotProduct(ToObject, Entry.Up);
	constexpr float FitMargin = 0.9f;
	if (FMath::Abs(LocalRight) > PortalHalfWidth * FitMargin ||
		FMath::Abs(LocalUp) > PortalHalfHeight * FitMargin)
	{
		return false;
	}

	// ...and it must actually be moving into the portal
	const float VelocityIntoPortal = -FVector::DotProduct(ObjectVelocity, Entry.Normal);
	if (VelocityIntoPortal < 30.f)
	{
		return false;
	}

	const FQuat EntryQuat = FRotationMatrix::MakeFromXZ(Entry.Normal, Entry.Up).ToQuat();
	const FQuat ExitQuat = FRotationMatrix::MakeFromXZ(Exit.Normal, Exit.Up).ToQuat();
	const FQuat PortalDelta = ExitQuat * FQuat(FVector::UpVector, PI) * EntryQuat.Inverse();

	const FVector TangentialOffset = Entry.Right * LocalRight + Entry.Up * LocalUp;
	const FVector ExitLocation = Exit.Location
		+ PortalDelta.RotateVector(TangentialOffset)
		+ Exit.Normal * (TeleportExitClearance + ObjectExtent);

	const FVector ExitVelocity = PortalDelta.RotateVector(ObjectVelocity);
	const FQuat ExitRotation = PortalDelta * Primitive->GetComponentQuat();

	if (Primitive == Actor->GetRootComponent())
	{
		Actor->SetActorLocationAndRotation(ExitLocation, ExitRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		Primitive->SetWorldLocationAndRotation(ExitLocation, ExitRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (Primitive->IsSimulatingPhysics())
	{
		Primitive->SetPhysicsLinearVelocity(ExitVelocity);
		Primitive->SetPhysicsAngularVelocityInDegrees(
			PortalDelta.RotateVector(Primitive->GetPhysicsAngularVelocityInDegrees()));
	}
	else if (ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->Velocity = ExitVelocity;
		}
	}

	if (USceneComponent* Root = Actor->GetRootComponent())
	{
		Root->ComponentVelocity = ExitVelocity;
	}

	// Stage 6: re-dress the object's ring + shimmer + glow so it keeps the portal look after teleporting
	SetPortalMaterialParams(
		Actor,
		GetPortalColor(TunnelId, EPortalType::PortalB),
		PortalShimmerTime * ShimmerSpeed,
		true,
		0.f);

	// Cooldown so the same object can't ping-pong instantly, plus housekeeping on dead entries
	ObjectTeleportCooldowns.Add(Actor, World->GetTimeSeconds() + ObjectTeleportCooldown);
	if (ObjectTeleportCooldowns.Num() > 64)
	{
		for (auto It = ObjectTeleportCooldowns.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent();
			}
		}
	}

	if (bShowTestOverlay && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			95,
			2.0f,
			FColor::Green,
			FString::Printf(TEXT("[Tunnel %d] %s went through"), TunnelId, *GetNameSafe(Actor)));
	}

	return true;
}

void APortalTargeter::HandleDebugSpawnBall(UWorld* World, const FVector& CameraLocation, const FVector& CameraForward)
{
	if (!World)
	{
		return;
	}

	const FVector SpawnLocation = CameraLocation + CameraForward * 150.f;
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	AStaticMeshActor* Ball = World->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!Ball)
	{
		return;
	}

	if (UStaticMeshComponent* Mesh = Ball->GetStaticMeshComponent())
	{
		UStaticMesh* BallMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		if (!BallMesh)
		{
			BallMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		}
		if (BallMesh)
		{
			Mesh->SetStaticMesh(BallMesh);
		}

		Mesh->SetMobility(EComponentMobility::Movable);
		Ball->SetActorScale3D(FVector(0.5f));
		Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
		Mesh->SetSimulatePhysics(true);
		Mesh->SetPhysicsLinearVelocity(CameraForward * 900.f);
	}
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
	const FColor BeamColor = GetPortalColor(Tunnels[TargetTunnel].TunnelId, ArmedType);
	DrawDebugLine(World, CameraLocation + CameraForward * 30.f, Placement.Location, BeamColor, false, 0.35f, 0, 3.0f);

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
		ConfigurePortalCapture(Slot.SpawnedActor, Slot, Tunnels[TunnelIndex].TunnelId, Type);
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

	// Drive ring + shimmer + glow (gracefully no-ops if BP_Portal's material lacks these params)
	SetPortalMaterialParams(PortalActor, Color, PortalShimmerTime * ShimmerSpeed, true, 0.f);

	// Portals are holes, not obstacles: they must never block the player
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	PortalActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		if (Primitive)
		{
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	// Add/configure colored glow light
	const FLinearColor LightColor = Color;
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
		LightComp->SetLightColor(LightColor);
		LightComp->SetIntensity(3000.f);
		LightComp->SetAttenuationRadius(350.f);
	}
}

void APortalTargeter::UpdateTunnelLinking(int32 TunnelIndex)
{
	if (!bShowTestOverlay)
	{
		return;
	}

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

void APortalTargeter::UpdatePortalCaptures(UWorld* World)
{
	if (!World || !bShowThroughPortals)
	{
		return;
	}

	auto UpdateOneCapture = [this, World](FPortalSlot& Slot, const FPortalSlot& Partner)
	{
		if (!Slot.bIsActive || !IsValid(Slot.SpawnedActor))
		{
			return;
		}

		USceneCaptureComponent2D* Capture = Slot.CaptureComponent;
		if (!IsValid(Capture))
		{
			return;
		}

		bool bShouldRender = true;

		// No linked partner yet -> nothing to show
		if (!Partner.bIsActive || !IsValid(Partner.SpawnedActor))
		{
			bShouldRender = false;
		}

		// Recursion guard: never render a portal's own view of itself
		if (bShouldRender && IsPortalSurfaceVisibleInAnotherCapture(World, Slot))
		{
			bShouldRender = false;
		}

		if (bShouldRender)
		{
			// Partner-frame view: the capture sits at the linked partner's portal and looks
			// outward through it, so the entry portal's surface shows the world beyond the exit.
			const FVector PartnerLoc = Partner.Location + Partner.Normal * 10.f;
			const FVector LookDir = Partner.Normal;

			Capture->SetWorldLocation(PartnerLoc);
			Capture->SetWorldRotation(LookDir.Rotation());

			if (Capture->ProjectionType == ECameraProjectionMode::Perspective)
			{
				Capture->FOVAngle = FMath::Clamp(Capture->FOVAngle, 40.f, 120.f);
			}

			// Clip everything behind the entry portal plane so the view stays inside the aperture
			Capture->bEnableClipPlane = true;
			Capture->ClipPlaneBase = Slot.Location - Slot.Normal * 1.0f;
			Capture->ClipPlaneNormal = Slot.Normal * -1.0f;

			Capture->TextureTarget = Slot.RenderTarget;

			if (IsValid(Slot.RenderTarget))
			{
				if (bPortalCaptureEveryFrame)
				{
					// bCaptureEveryFrame drives the update; a manual call here would be redundant
					Capture->SetComponentTickEnabled(true);
				}
				else
				{
					Capture->CaptureSceneDeferred();
				}
			}
		}
		else
		{
			// No partner or recursing: stop updating so the material keeps whatever it had
			Capture->SetComponentTickEnabled(false);
			Capture->TextureTarget = nullptr;
		}
	};

	for (FPortalTunnel& Tunnel : Tunnels)
	{
		UpdateOneCapture(Tunnel.SlotA, Tunnel.SlotB);
		UpdateOneCapture(Tunnel.SlotB, Tunnel.SlotA);
	}
}

void APortalTargeter::SetPortalViewTexture(AActor* PortalActor, UTextureRenderTarget2D* RenderTarget)
{
	if (!IsValid(PortalActor) || !RenderTarget)
	{
		return;
	}

	// Stage 7 material half: swap onto the dedicated portal-window material (created via
	// Source/AiHome/Tools/CreatePortalViewMaterial.py). One MID per portal so each binds
	// its own capture render target. Falls back to the original material if the asset
	// is missing.
	UMaterialInterface* PortalViewMatBase = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/Characters/M_PortalView.M_PortalView"));
	if (!PortalViewMatBase)
	{
		UE_LOG(LogTemp, Warning, TEXT("Portal view material not found (/Game/Characters/M_PortalView) - using PortalView param on original material"));
	}

	TArray<UMeshComponent*> MeshComponents;
	PortalActor->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (!Mesh)
		{
			continue;
		}

		const int32 NumMats = Mesh->GetNumMaterials();
		for (int32 MatIdx = 0; MatIdx < NumMats; ++MatIdx)
		{
			if (PortalViewMatBase)
			{
				UMaterialInstanceDynamic* ViewMat = UMaterialInstanceDynamic::Create(PortalViewMatBase, Mesh);
				if (ViewMat)
				{
					ViewMat->SetTextureParameterValue(FName("PortalView"), RenderTarget);
					Mesh->SetMaterial(MatIdx, ViewMat);
					continue;
				}
			}

			UMaterialInterface* CurrentMat = Mesh->GetMaterial(MatIdx);
			if (!CurrentMat)
			{
				continue;
			}

			UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(CurrentMat);
			if (!DynMat)
			{
				DynMat = UMaterialInstanceDynamic::Create(CurrentMat, Mesh);
				Mesh->SetMaterial(MatIdx, DynMat);
			}
			if (DynMat)
			{
				DynMat->SetTextureParameterValue(FName("PortalView"), RenderTarget);
			}
		}
	}
}

bool APortalTargeter::IsPortalSurfaceVisibleInAnotherCapture(UWorld* World, const FPortalSlot& Slot) const
{
	if (!World || !Slot.bIsActive || !IsValid(Slot.SpawnedActor))
	{
		return false;
	}

	// Recursion guard: if this same portal surface is visible through another live portal,
	// don't render its own capture this frame (avoids the infinite-funnel artifact).
	auto SamePortalSurface = [this, &Slot](const FPortalSlot& Other) -> bool
	{
		return Other.bIsActive
		 && IsValid(Other.SpawnedActor)
		 && Other.SpawnedActor != Slot.SpawnedActor
		 && FMath::Abs(FVector::DotProduct(Slot.Normal, Other.Normal)) > 0.99f
		 && FVector::Dist(Slot.Location, Other.Location) < 6.f;
	};

	for (const FPortalTunnel& Tunnel : Tunnels)
	{
		if (SamePortalSurface(Tunnel.SlotA) || SamePortalSurface(Tunnel.SlotB))
		{
			return true;
		}
	}

	return false;
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

void APortalTargeter::ConfigurePortalCapture(
	AActor* PortalActor,
	FPortalSlot& Slot,
	int32 TunnelId,
	EPortalType Type)
{
	if (!IsValid(PortalActor) || !GetWorld())
	{
		return;
	}

	// Find or create the render target + scene capture for this portal surface
	USceneCaptureComponent2D* Capture = Slot.CaptureComponent;
	UTextureRenderTarget2D* RenderTarget = Slot.RenderTarget;

	if (!IsValid(RenderTarget))
	{
		RenderTarget = NewObject<UTextureRenderTarget2D>(this);
		if (RenderTarget)
		{
			RenderTarget->InitAutoFormat(PortalRenderTargetSize, PortalRenderTargetSize);
			RenderTarget->UpdateResourceImmediate(true);
		}
	}

	if (!IsValid(Capture))
	{
		Capture = NewObject<USceneCaptureComponent2D>(PortalActor);
		if (Capture)
		{
			Capture->RegisterComponent();
			Capture->SetupAttachment(PortalActor->GetRootComponent(), NAME_None);
			Capture->SetMobility(EComponentMobility::Movable);
			Capture->bCaptureEveryFrame = bPortalCaptureEveryFrame;
			Capture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
			Capture->bEnableClipPlane = true;
			Capture->ProjectionType = ECameraProjectionMode::Perspective;
			Capture->bRenderInMainRenderer = false;
		}
	}

	if (!IsValid(Capture) || !IsValid(RenderTarget))
	{
		return;
	}

	Capture->TextureTarget = RenderTarget;
	Capture->SetRelativeLocation(FVector::ZeroVector);
	Capture->SetRelativeRotation(FRotator::ZeroRotator);

	// Remember the live objects so UpdatePortalCaptures can drive them
	Slot.CaptureComponent = Capture;
	Slot.RenderTarget = RenderTarget;

	int32 SlotIndex = (TunnelId == 1) ? 0 : 1;
	PortalRenderTargets[SlotIndex] = RenderTarget;
	PortalCaptureComponents[SlotIndex] = Capture;

	// Bind the render target to the portal mesh material (new PortalView param - graceful no-op if absent)
	SetPortalMaterialParams(
		PortalActor,
		GetPortalColor(TunnelId, Type),
		0.f,
		true,
		0.f);
	SetPortalViewTexture(PortalActor, RenderTarget);
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
