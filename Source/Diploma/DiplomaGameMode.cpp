// Copyright Epic Games, Inc. All Rights Reserved.

#include "DiplomaGameMode.h"
#include "DiplomaPawn.h"
#include "DiplomaHUD.h"

ADiplomaGameMode::ADiplomaGameMode()
{
	// set default pawn class to our flying pawn
	DefaultPawnClass = ADiplomaPawn::StaticClass();
	HUDClass = ADiplomaHUD::StaticClass();

}
