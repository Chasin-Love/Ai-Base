#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalTargeter.generated.h"

UCLASS()
class APortalTargeter : public AActor
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
	UPROPERTY(EditAnywhere, Category = "Portal")
	TSubclassOf<AActor> PortalClass;
};