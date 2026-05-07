#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T72TankProjectile.h"
#include "Components/SceneComponent.h"


#include "T72TankAIComponent.generated.h"

class UParticleSystem;
class UParticleSystemComponent;
class USceneComponent;
class UMaterialInterface;
class USkeletalMeshComponent;


UENUM(BlueprintType)
enum class ET72TankAIState : uint8
{
	Idle,
	MovingRoute,
	MovingFirePosition,
	Aiming,
	Firing,
	FirePositionPatrol,
	Relocating,
	Retreating,
	TrackTurnReaction,
	TrackFinalShot,
	TrackCrewEvacWait,
	Immobilized,
	Burning,
	Destroyed
};

UENUM(BlueprintType)
enum class ET72DamageZone : uint8
{
	None,
	GunBarrel,
	LeftTrack,
	RightTrack,
	Engine,
	TurretHatch,
	HullHatch
};

class AT72TankProjectile;
class USceneComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIPLOMA_API UT72TankAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT72TankAIComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 AI")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 AI")
	TArray<AActor*> RoutePoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 AI")
	TArray<AActor*> FirePositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 AI")
	AActor* FriendlyPosition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Movement")
	float MoveSpeedCm = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Movement")
	float BodyTurnSpeedDeg = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Movement")
	float AcceptanceRadiusCm = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Fire Patrol")
	bool bEnableFirePositionPatrol = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Fire Patrol")
	float FirePatrolLengthCm = 15000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Fire Patrol")
	float FirePatrolMoveSpeedCm = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Fire Patrol")
	bool bStartPatrolToRight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Fire Patrol")
	bool bFireBeforePatrolMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Animation")
	float WheelAnimationSpeedScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Weapon")
	float TurretTurnSpeedDeg = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Weapon")
	float GunElevationSpeedDeg = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Weapon")
	float FireAngleToleranceDeg = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Weapon")
	float FireInterval = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Weapon")
	float MinGunElevationDeg = -5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Weapon")
	float MaxGunElevationDeg = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile")
	TSubclassOf<AT72TankProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile")
	FName MuzzlePointComponentName = TEXT("MuzzlePoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile")
	bool bAimProjectileAtFriendlyPosition = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile")
	float ProjectileSpreadDeg = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Threat Reaction")
	bool bManeuverOnlyAfterThreat = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Threat Reaction")
	float ThreatReactionRadiusCm = 1000.0f;

	UPROPERTY(BlueprintReadOnly, Category = "T72 Threat Reaction")
	bool bThreatenedByDrone = false;

	UFUNCTION(BlueprintCallable, Category = "T72 Threat Reaction")
	void NotifyDroneImpactOrNearMiss(FVector ImpactLocation, bool bHitVulnerableZone);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Debug")
	bool bDebugLogs = true;

	UPROPERTY(BlueprintReadOnly, Category = "T72 State")
	ET72TankAIState CurrentState = ET72TankAIState::Idle;

	UFUNCTION(BlueprintCallable, Category = "T72 AI")
	void StartMission();

	UFUNCTION(BlueprintCallable, Category = "T72 AI")
	void StopMission();

	UFUNCTION(BlueprintCallable, Category = "T72 AI")
	void SetState(ET72TankAIState NewState);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Damage")
	FName PlayerDroneTag = TEXT("PlayerDrone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Damage")
	FName DroneWarheadComponentTag = TEXT("FPV_WarheadProbe");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Damage")
	float MinDirectHitSpeedCm = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Damage")
	float MinWarheadVelocityForwardDot = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Damage")
	float MinWarheadFacingZoneDot = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Damage")
	float EngineBurnoutTime = 180.0f;

	UPROPERTY(BlueprintReadOnly, Category = "T72 Damage")
	bool bGunDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "T72 Damage")
	bool bMobilityDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "T72 Damage")
	bool bEngineDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "T72 Damage")
	bool bTankDestroyed = false;

	UFUNCTION(BlueprintCallable, Category = "T72 Damage")
	void ApplyZoneDamage(ET72DamageZone Zone, FVector HitLocation);

	UFUNCTION(BlueprintCallable, Category = "T72 Damage")
	void NotifyDroneExplosion(FVector ImpactLocation, float ExplosionRadiusCm);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Engine Damage")
	float EngineMobilityFailureDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Engine Damage")
	float DamagedTurretTurnSpeedDeg = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Track Damage")
	float TrackReactionForwardSpeedScale = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Track Damage")
	float TrackTurnReactionDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Track Damage")
	float TrackTurnReactionSpeedDeg = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Track Damage")
	float TrackCrewEvacDelay = 20.0f;

	UPROPERTY(BlueprintReadOnly, Category = "T72 Damage")
	bool bEngineMobilityFailureApplied = false;

	UPROPERTY(BlueprintReadOnly, Category = "T72 Damage")
	bool bCrewEvacuated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	UParticleSystem* HatchKillFireFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	float HatchKillFireScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	FName EngineFXPointName = TEXT("EngineFXPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	FName CommandHatchSmokePointName = TEXT("CommandHatchSmokePoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	FName DriverHatchSmokePointName = TEXT("DriverHatchSmokePoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	FName TankExplosionPointName = TEXT("TankExplosionPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	UParticleSystem* EngineSecondaryExplosionFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	UParticleSystem* EngineFireFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	UParticleSystem* HatchSmokeFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	UParticleSystem* HatchKillExplosionFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	float EngineSecondaryExplosionDelay = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	float EngineSecondaryExplosionScale = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	float EngineFireScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	float HatchSmokeScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 FX")
	float HatchKillExplosionScale = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Destroyed Visual")
	UMaterialInterface* DestroyedTankMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Destroyed Visual")
	TArray<int32> DestroyedMaterialElementIndices;

private:

	float MovementDebugTimer = 0.0f;

	void ApplyDefaultAssetReferences();

	bool bTrackHitWhileMoving = false;
	bool IsMovementState(ET72TankAIState State) const;

	int32 CurrentRouteIndex = 0;
	int32 CurrentFirePositionIndex = 0;
	float FireTimer = 0.0f;
	float CurrentTurretAngle = 0.0f;
	float CurrentGunElevation = 0.0f;
	bool bMissionStarted = false;

	FVector FirePatrolCenter = FVector::ZeroVector;
	FVector FirePatrolAxis = FVector::ZeroVector;
	FVector FirePatrolTarget = FVector::ZeroVector;
	int32 FirePatrolDirection = 1;
	bool bFirePatrolInitialized = false;
	bool bFirePatrolFirstShotDone = false;

	void TickState(float DeltaTime);
	void TickMovementToActor(float DeltaTime, AActor* TargetActor);
	void TickFirePositionPatrol(float DeltaTime);
	void TickAiming(float DeltaTime);
	bool TickFiring(float DeltaTime);

	AActor* GetCurrentRouteTarget() const;
	AActor* GetCurrentFirePosition() const;

	void OnArrivedAtTarget();
	void BeginFirePositionPatrol();
	void SelectNextFirePosition();
	bool IsAimedAtFriendly() const;

	void SetWheelSpeed(float Speed);
	void SetTurretAngle(float Angle);
	void SetGunElevation(float Angle);
	void SetFiringActive(bool bActive);
	void PlayMuzzleFlash();
	void FireWeapon();
	USceneComponent* FindMuzzlePoint() const;

	bool CallBlueprintEventNoParams(FName EventName);
	bool CallBlueprintEventFloat(FName EventName, float Value);
	bool CallBlueprintEventBool(FName EventName, bool bValue);

	void DebugLog(const FString& Message) const;

	TMap<UPrimitiveComponent*, ET72DamageZone> DamageZoneMap;
	FTimerHandle EngineBurnoutTimerHandle;

	void RegisterDamageZones();
	ET72DamageZone GetDamageZoneByComponent(UPrimitiveComponent* Component) const;
	bool IsDroneActor(AActor* Actor) const;
	bool IsDroneWarheadComponent(UPrimitiveComponent* Component) const;
	bool IsValidDirectWarheadHit(UPrimitiveComponent* ZoneComponent, AActor* DroneActor, UPrimitiveComponent* DroneHitComponent, FVector& OutHitLocation) const;

	void DestroyTank();
	void ImmobilizeTank();
	void StartEngineBurning();
	void FinishEngineBurnout();
	void DisableGunAndRetreat();
	void OpenHatches();
	void SetShowDamage(bool bActive);
	void SetEngineFireActive(bool bActive);
	USkeletalMeshComponent* FindTankMesh() const;
	void ApplyDestroyedMaterial();

	FTimerHandle EngineMobilityFailureTimerHandle;
	FTimerHandle TrackTurnReactionTimerHandle;
	FTimerHandle TrackCrewEvacTimerHandle;

	ET72DamageZone DamagedTrackZone = ET72DamageZone::None;
	int32 TrackTurnDirection = 1;
	bool bTrackFinalShotFired = false;

	float GetEffectiveTurretTurnSpeedDeg() const;
	void ApplyEngineMobilityFailure();

	void StartTrackDamageSequence(ET72DamageZone Zone);
	void TickTrackTurnReaction(float DeltaTime);
	void FinishTrackTurnReaction();
	void TickTrackFinalShot(float DeltaTime);
	void StartTrackCrewEvacTimer();
	void FinishTrackCrewEvacTimer();


	FTimerHandle EngineSecondaryExplosionTimerHandle;
	FTimerHandle EngineHalfBurnTimerHandle;

	UParticleSystemComponent* EngineFirePSC = nullptr;
	UParticleSystemComponent* CommandHatchSmokePSC = nullptr;
	UParticleSystemComponent* DriverHatchSmokePSC = nullptr;
	UParticleSystemComponent* HatchKillFirePSC = nullptr;

;

	USceneComponent* FindSceneComponentByName(FName ComponentName) const;
	FVector GetFXLocation(FName PointName, FVector FallbackLocation) const;

	void PlayEngineSecondaryExplosion();
	void StartEngineFire();
	void TriggerEngineHalfBurnEffects();
	void StartHatchSmoke();
	void PlayHatchKillExplosion(FVector HitLocation);
	void FinalizeEngineBurnoutVisuals();
	void TriggerCrewEvacuation();
	void StartHatchKillFire();

	UFUNCTION()
	void OnDamageZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};