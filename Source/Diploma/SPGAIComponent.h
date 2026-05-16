#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T72TankProjectile.h"
#include "SPGAIComponent.generated.h"

class UAnimInstance;
class UMaterialInterface;
class UParticleSystem;
class UParticleSystemComponent;
class UPrimitiveComponent;
class USceneComponent;
class USkeletalMeshComponent;
class AFPVDronePawn;
class AInfantryCharacter;
class AInfantryAIController;
class UAPCAIComponent;

UENUM(BlueprintType)
enum class ESPGAIState : uint8
{
	Idle,
	MovingRoute,
	MovingFirePosition,
	Aiming,
	Firing,
	Returning,
	ReturnTurretToDefault,
	TrackTurnReaction,
	CrewEvacWait,
	Immobilized,
	EngineCoast,
	Burning,
	Destroyed
};

UENUM(BlueprintType)
enum class ESPGDamageZone : uint8
{
	None,
	LeftTrack,
	RightTrack,
	Engine,
	Turret,
	GunBarrel,
	Hull
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIPLOMA_API USPGAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USPGAIComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|AI")
	bool bAutoStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|AI")
	TArray<AActor*> RoutePoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|AI")
	AActor* FirePosition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|AI")
	AActor* FireTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Movement")
	float MoveSpeedCm = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Movement")
	float BodyTurnSpeedDeg = 38.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Movement")
	float AcceptanceRadiusCm = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Movement")
	float WheelAnimationSpeedScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	TSubclassOf<AT72TankProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	FName MuzzlePointComponentName = TEXT("MuzzlePoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	float TurretTurnSpeedDeg = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	float GunElevationSpeedDeg = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	float FireAngleToleranceDeg = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	float FireIntervalSeconds = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	int32 RequiredShotCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	float DefaultTurretYawDeg = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	float DefaultGunElevationDeg = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	float MinGunElevationDeg = -5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	float MaxGunElevationDeg = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	bool bAimProjectileAtFireTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Weapon")
	float ProjectileSpreadDeg = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Damage")
	FName PlayerDroneTag = TEXT("PlayerDrone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Damage")
	FName DroneWarheadComponentTag = TEXT("FPV_WarheadProbe");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Damage")
	bool bRequireDroneWarheadArmed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Damage")
	bool bCrashDroneOnVulnerableZoneContact = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Damage")
	float MinDirectHitSpeedCm = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Damage")
	float MinDroneZoneCrashSpeedCm = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "SPG|Damage")
	bool bGunDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "SPG|Damage")
	bool bMobilityDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "SPG|Damage")
	bool bEngineDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "SPG|Damage")
	bool bSPGDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "SPG|Crew")
	bool bCrewEvacuated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Engine Damage")
	float EngineMobilityFailureDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Engine Damage")
	float EngineCoastSpeedScale = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Track Damage")
	float TrackReactionForwardSpeedScale = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Track Damage")
	float TrackTurnReactionDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Track Damage")
	float TrackTurnReactionSpeedDeg = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Track Damage")
	float CrewEvacDelayAfterImmobilized = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	bool bFollowGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	bool bAlignToGround = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	bool bUseFourPointGroundTrace = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	bool bUseManualGroundTracePoints = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	float GroundTraceUpCm = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	float GroundTraceDownCm = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	float GroundOffsetCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	float GroundTraceHalfLengthCm = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	float GroundTraceHalfWidthCm = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	float MaxDriveableSlopeDeg = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	float GroundRotationInterpSpeedDeg = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	FName GroundTraceFLName = TEXT("GroundTrace_FL");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	FName GroundTraceFRName = TEXT("GroundTrace_FR");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	FName GroundTraceRLName = TEXT("GroundTrace_RL");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Ground")
	FName GroundTraceRRName = TEXT("GroundTrace_RR");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Animation")
	FName WheelSpeedVariableName = TEXT("WheelSpeed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Animation")
	FName TurretRotationVariableName = TEXT("TurretRotation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Animation")
	FName TurretElevationVariableName = TEXT("TurretElevation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Animation")
	FName HatchAngleVariableName = TEXT("HatchAngle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Animation")
	float OpenHatchAngle = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Animation")
	float HatchAnimSpeedDegPerSec = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Blueprint Events")
	FName SetFiringEventName = TEXT("SetFiring");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Blueprint Events")
	FName PlayMuzzleFlashEventName = TEXT("PlayMuzzleFlash");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName EngineFXPointName = TEXT("EngineFXPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName DestroyedFXPointName = TEXT("DestroyedFXPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName DestroyedExplosionFXPointName = TEXT("DestroyedExplosionFXPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName WreckageFXPointName = TEXT("WreckageFXPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName ShockWavePointName = TEXT("ShockWavePoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName NavObstacleComponentName = TEXT("NavObstacle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName PhysicsBlockerComponentName = TEXT("PhysicsBlocker");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName LeftTrackZoneComponentName = TEXT("SPG_LeftTrackZone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName RightTrackZoneComponentName = TEXT("SPG_RightTrackZone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName EngineZoneComponentName = TEXT("SPG_EngineZone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName TurretZoneComponentName = TEXT("SPG_TurretZone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName HullZoneComponentName = TEXT("SPG_HullZone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Components")
	FName GunBarrelZoneComponentName = TEXT("SPG_GunBarrelZone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	UParticleSystem* MuzzleFireFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	UParticleSystem* ShockWaveFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	UParticleSystem* EngineFireFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	UParticleSystem* DestroyedExplosionFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	UParticleSystem* DestroyedWreckageFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	UParticleSystem* TrackDustFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	float MuzzleFireScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	float ShockWaveScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	float EngineFireScale = .33f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	float DestroyedExplosionScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	float DestroyedWreckageScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	float DestroyedWreckageDelay = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	float TrackDustScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	float TrackDustMinWheelSpeed = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|FX")
	FString TrackDustComponentPrefix = TEXT("TrackDust");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Visual")
	UMaterialInterface* DestroyedMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Visual")
	bool bApplyDestroyedMaterialToAllSlots = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Visual")
	TArray<int32> DestroyedMaterialElementIndices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	TSubclassOf<AInfantryCharacter> CrewInfantryClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	AActor* CrewShelterPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	int32 CrewCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	float CrewSpawnIntervalSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	float CrewSpawnFallbackSpacingCm = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	float CrewSpawnZOffsetCm = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	float CrewSpawnSideOffsetCm = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	float CrewSpawnFrontOffsetCm = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	float CrewShelterFormationRadiusCm = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Crew")
	float CrewSpawnDelayAfterNavObstacleSeconds = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category = "SPG|Crew")
	TArray<AInfantryCharacter*> SpawnedCrew;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|APC Evacuation")
	TSubclassOf<AActor> APCClass;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SPG|APC Evacuation")
	AActor* APCSpawnPoint = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SPG|APC Evacuation")
	AActor* APCEvacPoint = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SPG|APC Evacuation")
	AActor* APCReturnPoint = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "SPG|APC Evacuation")
	AActor* APCBoardingMovePoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Debug")
	bool bDebugLogs = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Debug")
	bool bMovementDebugLogs = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SPG|Debug")
	float MovementDebugInterval = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category = "SPG|State")
	ESPGAIState CurrentState = ESPGAIState::Idle;

	UFUNCTION(BlueprintCallable, Category = "SPG|AI")
	void StartMission();

	UFUNCTION(BlueprintCallable, Category = "SPG|AI")
	void StopMission();

	UFUNCTION(BlueprintCallable, Category = "SPG|AI")
	void SetState(ESPGAIState NewState);

	UFUNCTION(BlueprintCallable, Category = "SPG|Damage")
	void ApplyZoneDamage(ESPGDamageZone Zone, FVector HitLocation);

	UFUNCTION(BlueprintCallable, Category = "SPG|Crew")
	int32 GetSPGCrewInsideCount() const;

	UFUNCTION(BlueprintCallable, Category = "SPG|Crew")
	int32 GetSpawnedCrewCount() const;

private:
	UPROPERTY()
	USkeletalMeshComponent* MeshComponent = nullptr;

	UPROPERTY()
	UAnimInstance* AnimInstance = nullptr;

	UPROPERTY()
	USceneComponent* MuzzlePoint = nullptr;

	UPROPERTY()
	USceneComponent* EngineFXPoint = nullptr;

	UPROPERTY()
	USceneComponent* DestroyedFXPoint = nullptr;

	UPROPERTY()
	USceneComponent* DestroyedExplosionFXPoint = nullptr;

	UPROPERTY()
	USceneComponent* WreckageFXPoint = nullptr;

	UPROPERTY()
	USceneComponent* ShockWavePoint = nullptr;

	UPROPERTY()
	UPrimitiveComponent* NavObstacleComponent = nullptr;

	UPROPERTY()
	UPrimitiveComponent* PhysicsBlockerComponent = nullptr;

	UPROPERTY()
	UParticleSystemComponent* EngineFirePSC = nullptr;

	UPROPERTY()
	TArray<UParticleSystemComponent*> TrackDustParticleComponents;

	UPROPERTY()
	AActor* SpawnedAPC = nullptr;

	TMap<UPrimitiveComponent*, ESPGDamageZone> DamageZoneMap;

	FTimerHandle EngineMobilityFailureTimerHandle;
	FTimerHandle TrackTurnReactionTimerHandle;
	FTimerHandle CrewEvacTimerHandle;
	FTimerHandle CrewSpawnDelayTimerHandle;
	FTimerHandle CrewSpawnStepTimerHandle;
	FTimerHandle DestroyedWreckageTimerHandle;

	int32 CurrentRouteIndex = 0;
	int32 CurrentReturnRouteIndex = 0;
	int32 FiredShots = 0;
	int32 PendingCrewSpawnIndex = 0;
	int32 SPGCrewInsideCount = 0;

	float FireTimer = 0.0f;
	float CurrentTurretRotation = 0.0f;
	float CurrentTurretElevation = 0.0f;
	float CurrentHatchAngle = 0.0f;
	float TargetHatchAngle = 0.0f;
	float MovementDebugTimer = 0.0f;
	float CurrentWheelSpeed = 0.0f;
	FVector PendingDestroyedWreckageLocation = FVector::ZeroVector;

	bool bMissionStarted = false;
	bool bTrackHitWhileMoving = false;
	bool bCrewSpawned = false;
	bool bAPCSpawned = false;
	bool bNavObstacleActive = false;

	ESPGDamageZone DamagedTrackZone = ESPGDamageZone::None;
	int32 TrackTurnDirection = 1;

	void ApplyDefaultAssetReferences();
	void CacheComponents();
	void CacheTrackDustComponents();
	UParticleSystemComponent* CreateTrackDustParticleComponent(USceneComponent* AttachPoint);
	void RegisterDamageZones();

	void TickState(float DeltaTime);
	void TickMovementToActor(float DeltaTime, AActor* TargetActor);
	void TickReturning(float DeltaTime);
	void TickAiming(float DeltaTime);
	void TickFiring(float DeltaTime);
	void TickTurretReturnToDefault(float DeltaTime);
	bool IsTurretAtDefault() const;
	void TickTrackTurnReaction(float DeltaTime);
	void TickEngineCoast(float DeltaTime);
	void TickHatchAnimation(float DeltaTime);

	void OnArrivedAtRouteTarget();
	void OnArrivedAtFirePosition();
	void BeginReturning();
	AActor* GetCurrentRouteTarget() const;
	AActor* GetCurrentReturnTarget() const;

	bool IsMovementState(ESPGAIState State) const;
	void GetDesiredWeaponAngles(float& OutTurretYawDeg, float& OutGunElevationDeg) const;
	bool IsAimedAtTarget() const;

	void FireWeapon();
	void DisableGunAndRetreat();
	void StartTrackDamageSequence(ESPGDamageZone Zone);
	void FinishTrackTurnReaction();
	void StartCrewEvacTimer();
	void FinishCrewEvacTimer();
	void StartEngineBurning();
	void ApplyEngineMobilityFailure();
	void DestroySPG(FVector HitLocation);

	void OpenHatches();

	void TriggerCrewEvacuation();
	void SpawnCrewFromSPGDelayed();
	void SpawnCrewFromSPG();
	void SpawnNextCrewMember();
	AInfantryCharacter* SpawnSingleCrewMember(int32 CrewIndex);
	FVector GetCrewSpawnLocation(int32 CrewIndex) const;
	FVector GetCrewShelterLocation(int32 CrewIndex) const;

	void SpawnAPCForCrewEvacuation();
	void UpdateAPCCrewAssignment();
	UAPCAIComponent* FindAPCAIComponent(AActor* APCActor) const;

	void SetNavObstacleActive(bool bActive);

	void SetWheelSpeed(float Speed);
	void SetTurretRotation(float Angle);
	void SetTurretElevation(float Angle);
	void SetFiringActive(bool bActive);
	void PlayMuzzleFlash();

	void SetAnimFloat(FName VariableName, float Value);
	bool CallBlueprintEventNoParams(FName EventName);
	bool CallBlueprintEventBool(FName EventName, bool bValue);

	bool IsDroneActor(AActor* Actor) const;
	bool IsDroneWarheadComponent(UPrimitiveComponent* Component) const;
	bool IsDroneWarheadArmed(AActor* DroneActor) const;
	bool IsValidDirectWarheadHit(UPrimitiveComponent* ZoneComponent, AActor* DroneActor, UPrimitiveComponent* DroneHitComponent, FVector& OutHitLocation) const;
	void CrashDroneOnVulnerableZoneContact(AActor* DroneActor, UPrimitiveComponent* ZoneComponent, UPrimitiveComponent* DroneHitComponent);

	ESPGDamageZone GetDamageZoneByComponent(UPrimitiveComponent* Component) const;

	bool GetManualGroundTraceOffsets(FVector& OutFL, FVector& OutFR, FVector& OutRL, FVector& OutRR) const;
	FVector TransformGroundTraceOffset(const FVector& DesiredLocation, float DesiredYaw, const FVector& LocalOffset) const;
	bool ProjectLocationToGround(const FVector& DesiredLocation, float DesiredYaw, FVector& OutLocation, FVector& OutGroundNormal, float& OutSlopeDeg) const;
	bool TraceGroundPoint(const FVector& WorldPoint, FVector& OutHitLocation, FVector& OutHitNormal) const;
	FRotator MakeGroundAlignedRotation(float DesiredYaw, const FVector& GroundNormal) const;
	bool IsSlopeDriveable(float SlopeDeg) const;

	USceneComponent* FindSceneComponentByName(FName ComponentName) const;
	UPrimitiveComponent* FindPrimitiveComponentByName(FName ComponentName) const;
	void RegisterDamageZoneByName(FName ComponentName, ESPGDamageZone Zone, FName OptionalTagName);
	void RegisterDamageZoneComponent(UPrimitiveComponent* Component, ESPGDamageZone Zone);
	FVector GetFXLocation(FName PointName, FVector FallbackLocation) const;
	void StartEngineFire();
	void PlayDestroyedExplosion(FVector HitLocation);
	void PlayDestroyedWreckage();
	void ApplyDestroyedMaterial();
	bool ApplyDestroyedMaterialToSlot(int32 MaterialIndex);
	void UpdateTrackDustVFX();
	bool IsTrackDustComponentName(const FString& ComponentName) const;
	void ClearDamageTimers();

	void DebugLog(const FString& Message) const;

	UFUNCTION()
	void OnDamageZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};