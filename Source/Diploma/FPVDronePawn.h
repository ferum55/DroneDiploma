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

UENUM(BlueprintType)
enum class EAutotuneAxis : uint8
{
    Roll,
    Pitch,
    Yaw
};

USTRUCT(BlueprintType)
struct FAutotuneSegment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    float Duration = 0.f;

    UPROPERTY(EditAnywhere)
    float Input = 0.f;
};

USTRUCT(BlueprintType)
struct FAutotuneMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float TrackingError = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float Overshoot = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float ReleaseSettling = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float OscillationPenalty = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float SaturationPenalty = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float Cost = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float ResponsePenalty = 0.f;
};

USTRUCT(BlueprintType)
struct FPIDCandidate
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    float P = 0.f;

    UPROPERTY(EditAnywhere)
    float I = 0.f;

    UPROPERTY(EditAnywhere)
    float D = 0.f;
};

UCLASS()
class AFPVDronePawn : public ADiplomaPawn
{
    GENERATED_BODY()

public:
    AFPVDronePawn();

    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

protected:
    virtual void ApplyThrust() override;
    virtual void ApplyTorques() override;

private:

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float DragCoeffForward = 0.00025f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float DragCoeffLateral = 0.00070f;

    UPROPERTY(EditAnywhere, Category = "FPV|Physics")
    float DragCoeffVertical = 0.005f;

    // Мотори: FL, FR, BL, BR
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    TArray<FMotorState> Motors;

    // Відстань від центру до мотора по X і Y (половина wheelbase)
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    float ArmLength = 13.f; // 130mm → 13cm → в Unreal units (см)

    // Максимальна тяга одного мотора в Ньютонах
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    float MaxMotorThrust = 20.f;

    // Коефіцієнт реактивного торку мотора (yaw від різниці CW/CCW)
    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    float MotorYawTorquePerNewton;

    UPROPERTY(EditAnywhere, Category = "FPV|Rates")
    float MaxPitchRate = 360.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Rates")
    float MaxRollRate = 360.f;

    UPROPERTY(EditAnywhere, Category = "FPV|Rates")
    float MaxYawRate = 180.f;

    // PID контролери
    UPROPERTY(EditAnywhere, Category = "FPV|PID")
    FPIDController PitchPID;

    UPROPERTY(EditAnywhere, Category = "FPV|PID")
    FPIDController RollPID;

    UPROPERTY(EditAnywhere, Category = "FPV|PID")
    FPIDController YawPID;


    UPROPERTY(EditAnywhere, Category = "FPV|ForwardFlight")
    float PropwashSpeedScaleMps = 30.f;

    UPROPERTY(EditAnywhere, Category = "FPV|ForwardFlight")
    float MinPropEfficiency = 0.85f;


    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorPropTorqueCoeff = 0.00000002f;

    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorResponseUp = 8.f;

    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorResponseDown = 6.f;

    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorThrustScale = 0.90f;

    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorTorquePerNewtonMeter = 0.032f;


    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorKV = 800.f;

    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorVoltageLoaded = 23.8f;

    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorResponseUpRPM = 14.f;

    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorResponseDownRPM = 10.f;

    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MotorMechanicalEfficiency = 0.85f;

    UPROPERTY(EditAnywhere, Category = "FPV|MotorModel")
    float MinOmegaRad = 30.f;

    float EvaluateMotorThrustGrams(float Command) const;
    float EvaluateMotorCurrentAmp(float Command) const;
    float EvaluateMotorPowerWatt(float Command) const;
    void UpdateMotorDynamics(float DeltaTime);
    float ComputePropEfficiencyFactor(const FVector& LocalVelocityMps) const;


    void InitMotors();
    void UpdateMotorThrusts(float DeltaTime);
    void ApplyMotorForces();
    void ApplyAerodynamicDrag();

    //debug
    FFPVDebugState DebugState;
    float DebugLogTimer = 0.f;
//Tuning PIDs
public:
    UFUNCTION(Exec)
    void TuneRollP();

    UFUNCTION(Exec)
    void TunePitchP();

    UFUNCTION(Exec)
    void TuneYawP();
private:
    bool bAutotuneActive = false;
    bool bAutotuneValidationPhase = false;
    bool IsAutotuneCandidateAcceptable(const FAutotuneMetrics& Metrics) const;
    float ReleasePhaseTime = 0.f;
    bool bWasInReleasePhase = false;

    EAutotuneAxis AutotuneAxis = EAutotuneAxis::Roll;

    TArray<FAutotuneSegment> AutotuneTrainScenario;
    TArray<FAutotuneSegment> AutotuneValidationScenario;

    TArray<FPIDCandidate> AutotuneCandidates;
    int32 AutotuneCandidateIndex = INDEX_NONE;

    FPIDCandidate CurrentCandidate;
    FPIDCandidate BestCandidate;

    FAutotuneMetrics CurrentMetrics;
    FAutotuneMetrics BestMetrics;

    float BestCost = TNumericLimits<float>::Max();

    int32 AutotuneSegmentIndex = 0;
    float AutotuneSegmentTime = 0.f;
    float AutotunePrevError = 0.f;

    float AutotuneThrottle = 0.30f;

    FVector AutotuneStartLocation = FVector::ZeroVector;
    FRotator AutotuneStartRotation = FRotator::ZeroRotator;

    float LastPitchCmd = 0.f;
    float LastRollCmd = 0.f;
    float LastYawCmd = 0.f;


    void BuildTrainScenario();
    void BuildValidationScenario();
    void BuildPCandidates(float MinP, float MaxP, float StepP);

    void StartAutotune(EAutotuneAxis Axis);
    void BeginAutotuneRun();
    void TickAutotune(float DeltaTime);
    void ResetAutotuneRunState();
    void ApplyPIDCandidate(const FPIDCandidate& Candidate);
    void SetAutotuneInputs(float AxisInput);
    float GetCurrentAxisRateNorm() const;
    float GetCurrentTargetRateNorm() const;
    void CollectAutotuneMetrics(float DeltaTime);
    float ComputeAutotuneCost(const FAutotuneMetrics& Metrics) const;
    void FinishAutotuneRun();
    void AdvanceAutotuneCandidate();
};