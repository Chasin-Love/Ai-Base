#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalPlacement.h"
#include "PortalTargeter.generated.h"

class UPrimitiveComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UENUM(BlueprintType)
enum class EPortalType : uint8
{
	PortalA UMETA(DisplayName = "Portal A"),
	PortalB UMETA(DisplayName = "Portal B")
};

USTRUCT(BlueprintType)
struct FPortalSlot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<AActor> SpawnedActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	FVector Normal = FVector::UpVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	FVector Right = FVector::RightVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	FVector Up = FVector::UpVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	bool bIsActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	double PlacedTime = 0.0;
};

USTRUCT(BlueprintType)
struct FPortalTunnel
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	int32 TunnelId = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	FPortalSlot SlotA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	FPortalSlot SlotB;

	bool IsLinked() const;
	bool HasAny() const;
	double GetOldestPlacedTime() const;
};

UCLASS()
class AIHOME_API APortalTargeter : public AActor
{
	GENERATED_BODY()

public:
	APortalTargeter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Portal Targeting")
	float TraceDistance = 10000.f;

	UPROPERTY(EditAnywhere, Category = "Portal Targeting", meta = (ClampMin = "1.0"))
	float PortalHalfWidth = 60.f;

	UPROPERTY(EditAnywhere, Category = "Portal Targeting", meta = (ClampMin = "1.0"))
	float PortalHalfHeight = 100.f;

	UPROPERTY(EditAnywhere, Category = "Portal Targeting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NormalDotMin = 0.92f;

	UPROPERTY(EditAnywhere, Category = "Portal Targeting", meta = (ClampMin = "0.0"))
	float PlaneTolerance = 12.f;

	/** If true, continuously draws Stage 1 debug circles on walls. Turned OFF by default for clean gameplay. */
	UPROPERTY(EditAnywhere, Category = "Portal Targeting")
	bool bContinuousDebugPreview = false;

	/** Stage 2 test overlay: armed/tunnel status lines, portal labels + rings and pairing
	 *  messages. Keep OFF for clean gameplay (only a failed shot or [Z] shows a message);
	 *  turn ON to see full portal diagnostics while testing. */
	UPROPERTY(EditAnywhere, Category = "Portal Targeting")
	bool bShowTestOverlay = false;

	/** Stage 3: linked portals carry the player through */
	UPROPERTY(EditAnywhere, Category = "Portal Teleport")
	bool bTeleportEnabled = true;

	/** How close (cm) the player's capsule surface must get to the portal plane to step through */
	UPROPERTY(EditAnywhere, Category = "Portal Teleport", meta = (ClampMin = "1.0"))
	float TeleportTriggerDistance = 25.f;

	/** How far in front of the exit portal the player is placed (cm) */
	UPROPERTY(EditAnywhere, Category = "Portal Teleport", meta = (ClampMin = "1.0"))
	float TeleportExitClearance = 80.f;

	/** Seconds before another teleport can happen (prevents instant re-entry) */
	UPROPERTY(EditAnywhere, Category = "Portal Teleport", meta = (ClampMin = "0.0"))
	float TeleportCooldown = 0.5f;

	/** Stage 4: physics props and other characters travel through portals too */
	UPROPERTY(EditAnywhere, Category = "Portal Teleport")
	bool bObjectsTeleportEnabled = true;

	/** Stage 5: brief screen flash at the moment of teleport */
	UPROPERTY(EditAnywhere, Category = "Portal Teleport")
	bool bTeleportFeedbackEnabled = true;

	/** Stage 6: bright ring around the portal aperture (Splitgate-style edge) */
	UPROPERTY(EditAnywhere, Category = "Portal Visual")
	bool bRenderRing = true;

	/** Ring width as a fraction of portal half-extents (0.0..1.0) */
	UPROPERTY(EditAnywhere, Category = "Portal Visual", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RingThickness = 0.18f;

	/** Shimmer/swirl animation speed (higher = faster) */
	UPROPERTY(EditAnywhere, Category = "Portal Visual", meta = (ClampMin = "0.0"))
	float ShimmerSpeed = 2.0f;

	/** Shimmer phase offset so the two portals in a pair look slightly different */
	UPROPERTY(EditAnywhere, Category = "Portal Visual", meta = (ClampMin = "0.0", ClampMax = "6.283185"))
	float ShimmerPhaseOffset = 0.0f;

	/** Emissive glow intensity multiplier on the portal surface */
	UPROPERTY(EditAnywhere, Category = "Portal Visual", meta = (ClampMin = "0.0"))
	float GlowEmissiveIntensity = 3.0f;

	/** Seconds before the same object can teleport again */
	UPROPERTY(EditAnywhere, Category = "Portal Teleport", meta = (ClampMin = "0.0"))
	float ObjectTeleportCooldown = 0.4f;

	/** Debug tool: press [B] to throw a physics ball at the portal. Turn off for a clean game. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugSpawnBallOnB = true;

	/** Portal blueprint/actor class to spawn (defaults to BP_Portal) */
	UPROPERTY(EditAnywhere, Category = "Portal")
	TSubclassOf<AActor> PortalClass;

	/** Currently armed portal type to fire */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Gun")
	EPortalType ArmedType = EPortalType::PortalA;

	/** Stage 5: draw a small crosshair while playing (aim aid) */
	UPROPERTY(EditAnywhere, Category = "Portal Gun")
	bool bCrosshairEnabled = true;

	/** The two portal tunnels managed by the player (Max 2 tunnels = 4 portals) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Gun")
	TArray<FPortalTunnel> Tunnels;

private:
	void HandleFire(UWorld* World, APlayerController* PC, const FVector& CameraLocation, const FVector& CameraForward);
	void HandleToggleArmedType();
	FCollisionQueryParams MakePlacementQueryParams(const APawn* MyPawn) const;

	void UpdatePortalTeleport(UWorld* World, APlayerController* PC);
	bool TryTeleportThroughPortal(const FPortalSlot& Entry, const FPortalSlot& Exit, int32 TunnelId, APlayerController* PC, APawn* Pawn);
	void TryTeleportObjectsThroughPortal(UWorld* World, const FPortalSlot& Entry, const FPortalSlot& Exit, int32 TunnelId, const APawn* PlayerPawn);
	bool TryTeleportObjectThroughPortal(UWorld* World, const FPortalSlot& Entry, const FPortalSlot& Exit, int32 TunnelId, UPrimitiveComponent* Primitive);
	void HandleDebugSpawnBall(UWorld* World, const FVector& CameraLocation, const FVector& CameraForward);
	FVector FindSafeExitLocation(UWorld* World, const FVector& DesiredLocation, const FVector& ExitNormal, float ProbeRadius) const;
	void UpdatePortalVisuals(UWorld* World, float DeltaTime);
	void UpdatePortalCaptures(UWorld* World);

	// Stage 6: drive ring + shimmer + glow on a portal actor
	void SetPortalMaterialParams(AActor* PortalActor, const FColor& Color, float ShimmerPhase, bool bIsLinked, float PortalPulse);

	// Stage 7: live view through the portal
	void ConfigurePortalCapture(AActor* PortalActor, const FPortalSlot& Slot, int32 TunnelId, EPortalType Type);
	void SetPortalViewTexture(AActor* PortalActor, UTextureRenderTarget2D* RenderTarget);

	int32 ResolveTargetTunnel(EPortalType TypeToPlace);
	void PlacePortalInSlot(int32 TunnelIndex, EPortalType Type, const FPortalPlacementResult& Placement);
	void UpdateTunnelLinking(int32 TunnelIndex);
	void ApplyPortalVisuals(AActor* PortalActor, int32 TunnelId, EPortalType Type);

	void RenderHUD();
	void RenderActivePortalsInWorld(UWorld* World);
	void RenderFailFeedback(UWorld* World);
	void DrawPlacementPreview(UWorld* World, const FVector& CameraLocation, const FVector& CameraForward, const FPortalPlacementResult& Placement) const;

public:
	static FColor GetPortalColor(int32 TunnelId, EPortalType Type);

private:
	// Stage 5 state
	bool bHudInitialized = false;
	float PortalVisualTime = 0.f;

	// Stage 6: per-portal shimmer phase so the pair can be offset
	float PortalShimmerTime = 0.f;

	// Feedback tracking
	float LastFailTime = -10.f;
	EPortalPlacementFail LastFailReason = EPortalPlacementFail::None;
	FVector LastFailLocation = FVector::ZeroVector;

	// Teleport tracking
	float TeleportCooldownUntil = 0.f;
	TMap<TWeakObjectPtr<AActor>, double> ObjectTeleportCooldowns;

	// Stage 7: live portal view
	UPROPERTY(EditAnywhere, Category = "Portal Visual")
	bool bShowThroughPortals = true;

	UPROPERTY(EditAnywhere, Category = "Portal Visual", meta = (ClampMin = "64", ClampMax = "2048"))
	int32 PortalRenderTargetSize = 512;

	UPROPERTY(EditAnywhere, Category = "Portal Visual")
	bool bPortalCaptureEveryFrame = false;

	// Stage 7: one render target + one capture per active portal surface
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Visual")
	TArray<TObjectPtr<UTextureRenderTarget2D>> PortalRenderTargets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Visual")
	TArray<TObjectPtr<USceneCaptureComponent2D>> PortalCaptureComponents;

	// Stage 7 helpers
	void UpdatePortalCaptures(UWorld* World);
	bool IsPortalSurfaceVisibleInAnotherCapture(UWorld* World, const FPortalSlot& Slot) const;
	void SetPortalViewTexture(AActor* PortalActor, UTextureRenderTarget2D* RenderTarget);
};

