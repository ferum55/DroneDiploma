#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FPVControllerSettingsTypes.h"
#include "FPVControllerSettingsSaveGame.generated.h"

UCLASS()
class DIPLOMA_API UFPVControllerSettingsSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category = "FPV|Controller")
    int32 Version = 1;

    UPROPERTY(VisibleAnywhere, Category = "FPV|Controller")
    FFPVControllerSettings Settings = FFPVControllerSettings::MakeDefault();
};
