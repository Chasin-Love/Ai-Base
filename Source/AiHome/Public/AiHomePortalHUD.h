#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AiHomePortalHUD.generated.h"

class APortalTargeter;

/**
 * Stage 5: a minimal in-game crosshair so the player can see where the portal beam will land.
 * Drawn only while a PortalTargeter is present and has its crosshair enabled.
 */
UCLASS()
class AIHOME_API AAiHomePortalHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	/** The portal targeter this HUD reads its state from (found once, cached) */
	TWeakObjectPtr<APortalTargeter> CachedTargeter;
};
