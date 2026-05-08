// Copyright Epic Games, Inc. All Rights Reserved.

#include "DiplomaGameMode.h"
#include "FPVDronePawn.h"
#include "DiplomaHUD.h"

ADiplomaGameMode::ADiplomaGameMode()
{
	// set default pawn class to our flying pawn
	DefaultPawnClass = APawn::StaticClass();
	HUDClass = ADiplomaHUD::StaticClass();

}
