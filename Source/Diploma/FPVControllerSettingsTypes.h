#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "FPVControllerSettingsTypes.generated.h"

USTRUCT(BlueprintType)
struct FFPVAxisControllerSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    float P = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    float I = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    float D = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    float DeadZone = 0.04f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    float Expo = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    float MaxRateDegPerSec = 360.0f;

    void Clamp()
    {
        P = FMath::Clamp(P, 0.0f, 1.0f);
        I = FMath::Clamp(I, 0.0f, 1.0f);
        D = FMath::Clamp(D, 0.0f, 1.0f);
        DeadZone = FMath::Clamp(DeadZone, 0.0f, 0.5f);
        Expo = FMath::Clamp(Expo, 1.0f, 3.0f);
        MaxRateDegPerSec = FMath::Clamp(MaxRateDegPerSec, 1.0f, 1000.0f);
    }
};

USTRUCT(BlueprintType)
struct FFPVControllerSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    FFPVAxisControllerSettings Pitch;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    FFPVAxisControllerSettings Roll;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    FFPVAxisControllerSettings Yaw;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    float ThrottleDeadZone = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    float ThrottleExpo = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    FKey ArmKey = EKeys::R;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    FKey BombArmKey = EKeys::B;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPV|Controller")
    FKey CycleFlightModeKey = EKeys::F;

    static FFPVControllerSettings MakeDefault()
    {
        FFPVControllerSettings Settings;

        Settings.Pitch.P = 0.25f;
        Settings.Pitch.I = 0.0f;
        Settings.Pitch.D = 0.01f;
        Settings.Pitch.DeadZone = 0.04f;
        Settings.Pitch.Expo = 1.0f;
        Settings.Pitch.MaxRateDegPerSec = 360.0f;

        Settings.Roll.P = 0.25f;
        Settings.Roll.I = 0.0f;
        Settings.Roll.D = 0.01f;
        Settings.Roll.DeadZone = 0.04f;
        Settings.Roll.Expo = 1.0f;
        Settings.Roll.MaxRateDegPerSec = 360.0f;

        Settings.Yaw.P = 1.0f;
        Settings.Yaw.I = 0.0f;
        Settings.Yaw.D = 0.075f;
        Settings.Yaw.DeadZone = 0.04f;
        Settings.Yaw.Expo = 1.0f;
        Settings.Yaw.MaxRateDegPerSec = 360.0f;

        Settings.ThrottleDeadZone = 0.02f;
        Settings.ThrottleExpo = 1.0f;
        Settings.ArmKey = EKeys::R;
        Settings.BombArmKey = EKeys::B;
        Settings.CycleFlightModeKey = EKeys::F;

        Settings.Clamp();
        return Settings;
    }

    void Clamp()
    {
        Pitch.Clamp();
        Roll.Clamp();
        Yaw.Clamp();
        ThrottleDeadZone = FMath::Clamp(ThrottleDeadZone, 0.0f, 0.5f);
        ThrottleExpo = FMath::Clamp(ThrottleExpo, 1.0f, 3.0f);

        if (!ArmKey.IsValid())
        {
            ArmKey = EKeys::R;
        }

        if (!BombArmKey.IsValid())
        {
            BombArmKey = EKeys::B;
        }

        if (!CycleFlightModeKey.IsValid())
        {
            CycleFlightModeKey = EKeys::F;
        }
    }
};
