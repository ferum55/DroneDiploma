#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPVFlightTypes.h"
#include "FPVFlightControllerComponent.generated.h"


class UStaticMeshComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIPLOMA_API UFPVFlightControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPVFlightControllerComponent();

	void ResetController();

	void CycleFlightMode();
	void UpdateMotorCommands(
		float DeltaTime,
		UStaticMeshComponent* Mesh,
		float Throttle,
		float PitchInput,
		float RollInput,
		float YawInput,
		bool bArmed,
		TArray<FMotorState>& Motors,
		FFPVDebugState& DebugState
	);

	EFPVFlightMode GetFlightMode() const { return FlightMode; }
	FString GetFlightModeText() const;

	float GetMaxPitchRate() const { return MaxPitchRate; }
	float GetMaxRollRate() const { return MaxRollRate; }
	float GetMaxYawRate() const { return MaxYawRate; }
	

private:
	UPROPERTY(EditAnywhere, Category = "FPV|Rates")
	float MaxPitchRate = 360.f;

	UPROPERTY(EditAnywhere, Category = "FPV|Rates")
	float MaxRollRate = 360.f;

	UPROPERTY(EditAnywhere, Category = "FPV|Rates")
	float MaxYawRate = 360.f;

	UPROPERTY(EditAnywhere, Category = "FPV|PID")
	FPIDController PitchPID;

	UPROPERTY(EditAnywhere, Category = "FPV|PID")
	FPIDController RollPID;

	UPROPERTY(EditAnywhere, Category = "FPV|PID")
	FPIDController YawPID;

	UPROPERTY(EditAnywhere, Category = "FPV|FlightMode")
	EFPVFlightMode FlightMode = EFPVFlightMode::Acro;

	UPROPERTY(EditAnywhere, Category = "FPV|FlightMode")
	float AngleMaxPitchDeg = 45.f;

	UPROPERTY(EditAnywhere, Category = "FPV|FlightMode")
	float AngleMaxRollDeg = 45.f;

	UPROPERTY(EditAnywhere, Category = "FPV|FlightMode")
	float AngleLevelGain = 8.f;

	UPROPERTY(EditAnywhere, Category = "FPV|FlightMode")
	float HorizonTransitionStart = 0.35f;

	UPROPERTY(EditAnywhere, Category = "FPV|FlightMode")
	float HorizonTransitionEnd = 0.85f;

	UPROPERTY(EditAnywhere, Category = "FPV|FlightMode")
	float AcroTrainerMaxPitchDeg = 55.f;

	UPROPERTY(EditAnywhere, Category = "FPV|FlightMode")
	float AcroTrainerMaxRollDeg = 55.f;

	UPROPERTY(EditAnywhere, Category = "FPV|FlightMode")
	float AcroTrainerReturnGain = 10.f;


	UPROPERTY(EditAnywhere, Category = "FPV|Debug")
	bool bLogFlightModeDebug = true;

	UPROPERTY(EditAnywhere, Category = "FPV|Debug")
	float FlightModeDebugInterval = 0.10f;

	float FlightModeDebugTimer = 0.f;
	float DebugLogTimer = 0.f;

	float ComputeAngleRateNorm(float TargetAngleDeg, float CurrentAngleDeg, float MaxRateDeg) const;
	float ApplyAcroTrainerLimit(float TargetRateNorm, float CurrentAngleDeg, float LimitDeg, float MaxRateDeg) const;
};