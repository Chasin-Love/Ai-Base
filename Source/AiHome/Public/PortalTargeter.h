#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalPlacement.h"
#include "PortalTargeter.generated.h"

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

	/** Portal blueprint/actor class to spawn (defaults to BP_Portal) */
	UPROPERTY(EditAnywhere, Category = "Portal")
	TSubclassOf<AActor> PortalClass;

	/** Currently armed portal type to fire */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Gun")
	EPortalType ArmedType = EPortalType::PortalA;

	/** The two portal tunnels managed by the player (Max 2 tunnels = 4 portals) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Gun")
	TArray<FPortalTunnel> Tunnels;

private:
	void HandleFire(UWorld* World, APlayerController* PC, const FVector& CameraLocation, const FVector& CameraForward);
	void HandleToggleArmedType();
	FCollisionQueryParams MakePlacementQueryParams(const APawn* MyPawn) const;

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
	// Feedback tracking
	float LastFailTime = -10.f;
	EPortalPlacementFail LastFailReason = EPortalPlacementFail::None;
	FVector LastFailLocation = FVector::ZeroVector;
};
