// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AiHomeGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AAiHomeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAiHomeGameMode();
};



