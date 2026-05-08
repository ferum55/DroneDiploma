#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Engine/EngineTypes.h"
#include "DroneTelemetry.h"
#include "FPVDronePawn.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UInputComponent;
class UMaterialInstanceDynamic;
class UParticleSystem;
class UFPVBatteryComponent;
class UDroneSignalComponent;


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

UCLASS(Config = Game)
class DIPLOMA_API AFPVDronePawn : public APawn
{
    GENERATED_BODY()

public:
    AFPVDronePawn();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

    FORCEINLINE UCameraComponent* GetCamera() const { return Camera; }
    FORCEINLINE UStaticMeshComponent* GetPlaneMesh() const { return PlaneMesh; }

    float GetRollInput() const { return RollInput; }
    float GetPitchInput() const { return PitchInput; }
    float GetYawInput() const { return YawInput; }

    float GetReceivedThrottle() const;
    float GetReceivedPitchInput() const;
    float GetReceivedRollInput() const;
    float GetReceivedYawInput() const;
    float GetControlInputScale() const;
    bool IsControlFailsafeActive() const;

    const FDroneTelemetry& GetTelemetry() const { return Telemetry; }

    bool IsArmed() const { return bArmedState; }
    bool IsBombArmed() const { return bBombArmedState; }
    bool IsKillCamActive() const { return bKillCamActive; }
    bool IsCrashed() const { return bCrashed; }

    float GetBatteryLoadedVoltage() const;
    float GetBatteryConsumedAh() const;
    float GetBatteryTotalCurrentA() const;

    void UpdateTelemetry();

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    UPROPERTY(Category = Mesh, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UStaticMeshComponent* PlaneMesh = nullptr;

    UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* Camera = nullptr;

    UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* KillCamCamera = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPV|Battery", meta = (AllowPrivateAccess = "true"))
    UFPVBatteryComponent* BatteryComponent = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UAV|Signal", meta = (AllowPrivateAccess = "true"))
    UDroneSignalComponent* SignalComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    FDroneTelemetry Telemetry;

    UPROPERTY()
    UMaterialInstanceDynamic* FPVPostProcessMID = nullptr;

    bool bArmedState = false;
    bool bBombArmedState = false;

    float TelemetryStartTimeSeconds = 0.f;
    float Throttle = 0.f;
    float PitchInput = 0.f;
    float RollInput = 0.f;
    float YawInput = 0.f;
    float BaroZeroZ = 0.f;
    float LastDeltaSeconds = 0.f;

    bool bCrashed = false;
    FVector SpawnLocation;
    FRotator SpawnRotation;
    float LastSpawnWorldTime = 0.f;

    UPROPERTY(EditAnywhere, Category = "UAV|Crash")
    float KillCamDuration = 3.f;

    UPROPERTY(EditAnywhere, Category = "UAV|Crash")
    UParticleSystem* ExplosionEffect = nullptr;

    UPROPERTY(EditAnywhere, Category = "UAV|Crash")
    float CrashSpawnGraceSeconds = 1.0f;

    UPROPERTY(EditAnywhere, Category = "UAV|Crash")
    float CrashMinImpactSpeedMps = 8.0f;

    UPROPERTY(EditAnywhere, Category = "UAV|Crash")
    float CrashMinNormalImpulse = 25000.0f;

    bool bKillCamActive = false;
    float KillCamTimer = 0.f;
    FVector CrashLocation;
    FVector KillCamLocation;
    bool bKillCamExplosionPending = false;
    bool bKillCamExplosionSpawned = false;

    UPROPERTY(EditAnywhere, Category = "FPV|Debug")
    bool bLogFlightModeDebug = true;

    UPROPERTY(EditAnywhere, Category = "FPV|Debug")
    float FlightModeDebugInterval = 0.10f;

    float FlightModeDebugTimer = 0.f;

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

    UPROPERTY(EditAnywhere, Category = "FPV|Motors")
    TArray<FMotorState> Motors;

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

    UPROPERTY(EditAnywhere, Category = "FPV|Explosion")
    float ExplosionDamage = 125.0f;

    UPROPERTY(EditAnywhere, Category = "FPV|Explosion")
    float ExplosionMinimumDamage = 100.0f;

    UPROPERTY(EditAnywhere, Category = "FPV|Explosion")
    float ExplosionInnerRadius = 250.0f;

    UPROPERTY(EditAnywhere, Category = "FPV|Explosion")
    float ExplosionOuterRadius = 650.0f;

    UPROPERTY(EditAnywhere, Category = "FPV|Explosion")
    float ExplosionDamageFalloff = 1.0f;

    FFPVDebugState DebugState;
    float DebugLogTimer = 0.f;

    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    void ToggleArm();
    void ToggleBombArm();
    void ThrottleInput(float Value);
    void PitchInputAxis(float Value);
    void RollInputAxis(float Value);
    void YawInputAxis(float Value);

    float NormalizeThrottle(float Raw) const;
    float NormalizeCenteredAxis(float Raw) const;

    void UpdateBaseTelemetry();
    float GetRadioAltitudeMeters(bool& bValid) const;

    void UpdateSignalTelemetry(float DeltaTime);

    bool ShouldIgnoreCrashHit(const FVector& NormalImpulse) const;
    void HandleCrashExplosion(const FHitResult& Hit);
    void HandleCrash(const FVector& HitLocation);
    void SpawnCrashExplosion(const FVector& HitLocation);
    void StartKillCam(const FVector& HitLocation);
    void EndKillCam();
    void UpdateKillCamReplay(float DeltaSeconds);
    void ResetDroneStateAfterRespawn();

    void ApplyThrust();
    void InitMotors();
    void UpdateMotorThrusts(float DeltaTime);
    void UpdateMotorDynamics(float DeltaTime);
    float ComputePropEfficiencyFactor(const FVector& LocalVelocityMps) const;
    void ApplyMotorForces();
    void ApplyAerodynamicDrag();

    void ApplyExplosionDamage(FVector ExplosionLocation);
    void CycleFlightMode();
    FString GetFlightModeText() const;
    float ComputeAngleRateNorm(float TargetAngleDeg, float CurrentAngleDeg, float MaxRateDeg) const;
    float ApplyAcroTrainerLimit(float TargetRateNorm, float CurrentAngleDeg, float LimitDeg, float MaxRateDeg) const;
};
