#pragma once

#include "CoreMinimal.h"
#include "FPVFlightTypes.generated.h"

UENUM(BlueprintType)
enum class EFPVFlightMode : uint8
{
	Acro,
	Angle,
	Horizon,
	AcroTrainer
};

USTRUCT(BlueprintType)
struct FPIDController
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float P = 1.f;

	UPROPERTY(EditAnywhere)
	float I = 0.f;

	UPROPERTY(EditAnywhere)
	float D = 0.f;

	float Integral = 0.f;
	float PrevError = 0.f;

	UPROPERTY(EditAnywhere)
	float IntegralClamp = 1.f;

	float Update(float Target, float Current, float DeltaTime)
	{
		const float SafeDeltaTime = FMath::Max(DeltaTime, KINDA_SMALL_NUMBER);
		const float Error = Target - Current;
		Integral = FMath::Clamp(Integral + Error * SafeDeltaTime, -IntegralClamp, IntegralClamp);
		const float Derivative = (Error - PrevError) / SafeDeltaTime;
		PrevError = Error;
		return P * Error + I * Integral + D * Derivative;
	}

	void Reset()
	{
		Integral = 0.f;
		PrevError = 0.f;
	}
};

USTRUCT(BlueprintType)
struct FMotorState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FVector LocalPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere)
	float SpinDirection = 1.f;

	float Command = 0.f;
	float CurrentCommand = 0.f;

	float TargetRPM = 0.f;
	float CurrentRPM = 0.f;

	float ThrustNewton = 0.f;
	float CurrentDrawAmp = 0.f;
	float ElectricalPowerWatt = 0.f;
	float MechanicalPowerWatt = 0.f;
	float ReactionTorqueNm = 0.f;
};

USTRUCT()
struct FFPVDebugState
{
	GENERATED_BODY()

	float CurrentPitchRate = 0.f;
	float CurrentRollRate = 0.f;
	float CurrentYawRate = 0.f;

	float TargetPitchRate = 0.f;
	float TargetRollRate = 0.f;
	float TargetYawRate = 0.f;

	float PitchCmd = 0.f;
	float RollCmd = 0.f;
	float YawCmd = 0.f;

	float FL = 0.f;
	float FR = 0.f;
	float BL = 0.f;
	float BR = 0.f;

	float Throttle = 0.f;
	float PitchInput = 0.f;
	float RollInput = 0.f;
	float YawInput = 0.f;
};