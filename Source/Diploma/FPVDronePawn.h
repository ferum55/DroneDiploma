#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Engine/EngineTypes.h"
#include "DroneTelemetry.h"
#include "FPVFlightTypes.h"
#include "FPVDronePawn.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UInputComponent;
class UMaterialInstanceDynamic;
class UParticleSystem;
class UFPVBatteryComponent;
class UDroneSignalComponent;
class UFPVMotorComponent;
class UFPVFlightControllerComponent;

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
    UFUNCTION(BlueprintCallable, Category = "FPV")
    void ForceCrashAtLocation(const FVector& HitLocation);

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    bool bUseManualExplosionDamage = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    float ManualExplosionMaxDamage = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    float ManualExplosionMinDamage = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    float ManualExplosionInnerRadiusCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    float ManualExplosionOuterRadiusCm = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    float ManualExplosionFalloff = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion")
    float ExplosionDamageOriginZOffset = 80.0f;
    //

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPV|Motors", meta = (AllowPrivateAccess = "true"))
    UFPVMotorComponent* MotorComponent = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPV|FlightController", meta = (AllowPrivateAccess = "true"))
    UFPVFlightControllerComponent* FlightControllerComponent = nullptr;

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
    float CrashBombArmedMinImpactSpeedKmh = 10.0f;

    UPROPERTY(EditAnywhere, Category = "UAV|Crash")
    float CrashUnarmedMinImpactSpeedKmh = 20.0f;

    UPROPERTY(EditAnywhere, Category = "UAV|Crash")
    FName InfantryBulletTag = TEXT("InfantryBullet");

    bool bKillCamActive = false;
    float KillCamTimer = 0.f;
    FVector CrashLocation;
    FVector KillCamLocation;
    bool bKillCamExplosionPending = false;
    bool bKillCamExplosionSpawned = false;

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

    bool ShouldIgnoreCrashHit(AActor* OtherActor, const FVector& NormalImpulse) const;
    bool IsInfantryBulletActor(const AActor* OtherActor) const;
    float GetCurrentSpeedKmh() const;

    void HandleCrashExplosion(const FHitResult& Hit);
    void HandleCrash(const FVector& HitLocation);
    void SpawnCrashExplosion(const FVector& HitLocation);
    void StartKillCam(const FVector& HitLocation);
    void EndKillCam();
    void UpdateKillCamReplay(float DeltaSeconds);
    void ResetDroneStateAfterRespawn();


    TArray<FMotorState>& GetActiveMotors();
    const TArray<FMotorState>& GetActiveMotors() const;
    float GetActiveArmX() const;
    float GetActiveArmY() const;

    void ApplyThrust();
    void UpdateMotorThrusts(float DeltaTime);
    void UpdateMotorDynamics(float DeltaTime);
    void ApplyMotorForces();
    void ApplyAerodynamicDrag();

    void ApplyExplosionDamage(FVector ExplosionLocation);
    void CycleFlightMode();
    FString GetFlightModeText() const;

    //Switches
    bool bArmSwitchInitialized = false;
    bool bLastArmSwitchOn = false;

    bool bBombSwitchInitialized = false;
    bool bLastBombSwitchOn = false;

    bool bModeSwitchInitialized = false;
    int32 LastModeSwitchPosition = 0;
    int32 CurrentModeSwitchPosition = 0;

    bool bTrainerSwitchInitialized = false;
    bool bLastTrainerSwitchOn = false;
    bool bCurrentTrainerSwitchOn = false;

    void SetArmState(bool bNewState);
    void SetBombArmState(bool bNewState);

    void ArmSwitchAxis(float Value);
    void BombArmSwitchAxis(float Value);
    void FlightModeSwitchAxis(float Value);
    void AcroTrainerSwitchAxis(float Value);

    bool ReadTwoPositionSwitch(float Value) const;
    int32 ReadThreePositionSwitch(float Value) const;

    void ApplyFlightModeFromSwitches();
    void SetFlightModeDirect(EFPVFlightMode NewMode);
    };
