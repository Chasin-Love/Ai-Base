#include "AiHomePortalHUD.h"
#include "PortalTargeter.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"

void AAiHomePortalHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas || !GetWorld())
	{
		return;
	}

	// Find the portal targeter once and keep a weak reference to it
	if (!CachedTargeter.IsValid())
	{
		for (TActorIterator<APortalTargeter> It(GetWorld()); It; ++It)
		{
			CachedTargeter = *It;
			break;
		}
	}

	const APortalTargeter* Targeter = CachedTargeter.Get();
	if (!Targeter || !Targeter->bCrosshairEnabled)
	{
		return;
	}

	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;

	// Small four-line reticle with a gap in the middle
	constexpr float Gap = 5.f;
	constexpr float Length = 9.f;
	constexpr float Thickness = 1.5f;
	const FLinearColor LineColor(0.9f, 0.95f, 1.f, 0.85f);

	DrawLine(CenterX - Gap - Length, CenterY, CenterX - Gap, CenterY, LineColor, Thickness);
	DrawLine(CenterX + Gap, CenterY, CenterX + Gap + Length, CenterY, LineColor, Thickness);
	DrawLine(CenterX, CenterY - Gap - Length, CenterX, CenterY - Gap, LineColor, Thickness);
	DrawLine(CenterX, CenterY + Gap, CenterX, CenterY + Gap + Length, LineColor, Thickness);

	// Center dot tinted with the currently armed portal color
	const FColor ArmedColor = APortalTargeter::GetPortalColor(1, Targeter->ArmedType);
	DrawRect(FLinearColor(ArmedColor), CenterX - 2.f, CenterY - 2.f, 4.f, 4.f);
}
