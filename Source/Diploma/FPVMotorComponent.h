#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPVFlightTypes.h"
#include "FPVMotorComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIPLOMA_API UFPVMotorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPVMotorComponent();

	void InitMotors();
	void InitMotors(float InArmX, float InArmY);

	void ResetMotors();

	const TArray<FMotorState>& GetMotors() const { return Motors; }
	TArray<FMotorState>& GetMutableMotors() { return Motors; }

	float GetArmX() const { return ArmX; }
	float GetArmY() const { return ArmY; }
	float GetTotalCurrentA() const { return TotalCurrentA; }
	float GetTotalThrustN() const { return TotalThrustN; }
	float GetTotalElectricalPowerW() const { return TotalElectricalPowerW; }
	float GetAverageRPM() const { return AverageRPM; }
	float GetLastPropEfficiencyFactor() const { return LastPropEfficiencyFactor; }

	void UpdateMotorDynamics(
		float DeltaTime,
		float BatteryLoadedVoltage,
		float BatteryOutputScale,
		float BatteryBenchReferenceVoltage,
		const FVector& LocalVelocityMps
	);


private:
	UPROPERTY(EditAnywhere, Category = "FPV|Motors")
	TArray<FMotorState> Motors;

	UPROPERTY(EditAnywhere, Category = "FPV|Motors")
	float ArmX = 16.67f;

	UPROPERTY(EditAnywhere, Category = "FPV|Motors")
	float ArmY = 14.5f;

	UPROPERTY(EditAnywhere, Category = "FPV|ForwardFlight")
	float PropwashSpeedScaleMps = 22.f;

	UPROPERTY(EditAnywhere, Category = "FPV|ForwardFlight")
	float MinPropEfficiency = 0.68f;

	UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
	float MotorThrustScale = 0.90f;

	UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
	float MotorKV = 900.f;

	UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
	float MotorResponseUpRPM = 14.f;

	UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
	float MotorResponseDownRPM = 10.f;

	UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
	float MotorMechanicalEfficiency = 0.85f;

	UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
	float MinOmegaRad = 30.f;

	float TotalCurrentA = 0.f;
	float TotalThrustN = 0.f;
	float TotalElectricalPowerW = 0.f;
	float AverageRPM = 0.f;
	float LastPropEfficiencyFactor = 1.f;

	float ComputePropEfficiencyFactor(const FVector& LocalVelocityMps) const;
};