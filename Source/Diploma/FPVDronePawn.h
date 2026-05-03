#pragma once

#include "CoreMinimal.h"
#include "Misc/AssertionMacros.h"
#include "DiplomaPawn.h"
#include "FPVDronePawn.generated.h"

USTRUCT(BlueprintType)
struct FPIDController
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere) float P = 1.f;
    UPROPERTY(EditAnywhere) float I = 0.f;
    UPROPERTY(EditAnywhere) float D = 0.f;

    float Integral = 0.f;
    float PrevError = 0.f;

    UPROPERTY(EditAnywhere) float IntegralClamp = 1.f;

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

UCLASS()
class AFPVDronePawn : public ADiplomaPawn
{
    GENERATED_BODY()

public:
    AFPVDronePawn();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

    //BatteryTelemetry
    float GetBatteryLoadedVoltage() const { return BatteryLoadedVoltage; }
    float GetBatteryConsumedAh() const { return BatteryConsumedAh; }
    float GetBatteryTotalCurrentA() const { return BatteryTotalCurrentA; }

protected:
    virtual void ApplyThrust() override;
    virtual void ApplyTorques() override;
    virtual void UpdateTelemetry() override;
    virtual void ResetDroneStateAfterRespawn() override;

private:


    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float AirDensity = 1.225f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float CdForward = 1.5f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float CdLateral = 2.5f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float CdVertical = 2.4f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float AreaForward = 0.003916f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float AreaLateral = 0.013811f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float AreaVertical = 0.053622f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float RotorDiscDiameterM = 0.254f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float RotorVerticalAreaFactor = 0.225f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float RotorVerticalCd = 1.2f;

    // Мотори: FL, FR, BL, BR
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    TArray<FMotorState> Motors;

    // Відстань від центру до мотора по X і Y (половина wheelbase)
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    float ArmX = 0.f;
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    float ArmY = 0.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Rates")
    float MaxPitchRate = 0.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Rates")
    float MaxRollRate = 0.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Rates")
    float MaxYawRate = 0.f;

    // PID контролери
    UPROPERTY(EditAnywhere, Category = "FPV|PID")
    FPIDController PitchPID;

    UPROPERTY(EditAnywhere, Category = "FPV|PID")
    FPIDController RollPID;

    UPROPERTY(EditAnywhere, Category = "FPV|PID")
    FPIDController YawPID;


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

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    int32 BatterySeriesCells = 6;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    int32 BatteryParallelCells = 3;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryCellCapacityAh = 5.0f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryCellInternalResistanceOhm = 0.012f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryCellVoltageFull = 4.2f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryCellVoltageNominal = 3.6f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryCellVoltageEmpty = 3.0f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryUsableFraction = 0.85f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryBenchReferenceVoltage = 25.2f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryPackMassKg = 1.28f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryConsumedAh = 0.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatterySoC = 1.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryOpenCircuitVoltage = 25.2f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryLoadedVoltage = 25.2f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryTotalCurrentA = 0.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryCellVoltageWarn = 3.30f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryCellVoltageCritical = 3.10f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryCellVoltageCutoff = 2.95f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryResistanceScale = 1.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    float BatteryOutputScale = 1.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    bool bBatteryLowVoltageWarn = false;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    bool bBatteryCriticalVoltage = false;

    UPROPERTY(EditAnywhere, Category = "FPV|Battery")
    bool bBatteryCutoffActive = false;

    /*UPROPERTY(EditAnywhere, Category = "FPV|Telemetry")
    float VideoLinkPercentValue = 100.f;*/

    UPROPERTY(EditAnywhere, Category = "FPV|Telemetry")
    float TxPowerWValue = 2.5f;

    UPROPERTY(EditAnywhere, Category = "FPV|Payload")
    bool bBombArmed = false;

    /*float EvaluateMotorCurrentAmp(float Command) const;
    float EvaluateMotorThrustGramsFromCurrent(float CurrentAmp) const;
    float EvaluateMotorPowerWattFromCurrent(float CurrentAmp) const;*/
    void UpdateMotorDynamics(float DeltaTime);
    float ComputePropEfficiencyFactor(const FVector& LocalVelocityMps) const;


    void InitMotors();
    void UpdateMotorThrusts(float DeltaTime);
    void ApplyMotorForces();
    void ApplyAerodynamicDrag();

    //Battery
    float GetBatteryCapacityAh() const;
    float GetBatteryUsableCapacityAh() const;
    float GetBatteryInternalResistanceOhm() const;
    float EvaluateCellOCVFromSoC(float SoC) const;
    void ResetBatteryState();
    void UpdateBatteryState(float TotalCurrentA, float DeltaTime);
    float EvaluateBatteryResistanceScaleFromSoC(float SoC) const;
    float EvaluateBatteryOutputScaleFromCellVoltage(float CellLoadedVoltage) const;

    //debug
    FFPVDebugState DebugState;
    float DebugLogTimer = 0.f;
};