// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArenaShooterGameMode.h"
#include "ArenaShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"

AArenaShooterGameMode::AArenaShooterGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
