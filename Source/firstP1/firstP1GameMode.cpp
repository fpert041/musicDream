// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "firstP1.h"
#include "firstP1GameMode.h"
#include "firstP1HUD.h"
#include "firstP1Character.h"

AfirstP1GameMode::AfirstP1GameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AfirstP1HUD::StaticClass();
}
