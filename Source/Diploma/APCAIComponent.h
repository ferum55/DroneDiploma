#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "APCAIComponent.generated.h"

class UPrimitiveComponent;
class USkeletalMeshComponent;
class UParticleSystemComponent;
class UMaterialInterface;
class UNiagaraSystem;
class UNiagaraComponent;
class AFPVDronePawn;
class AInfantryCharacter;
class AInfantryAIController;

UENUM(BlueprintType)
enum class EAPCAIState : uint8
{
	Inactive,
	MoveToEvacPoint,
	OpeningRearDoors,
	WaitingForCrew,
	ClosingRearDoors,
	Returning,
	Destroyed
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DIPLOMA_API UAPCAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAPCAIComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "APC")
	void StartEvacuation(AActor* InEvacPoint, AActor* InReturnPoint);

	UFUNCTION(BlueprintCallable, Category = "APC")
	void DestroyAPC(const FVector& HitLocation);

	UFUNCTION(BlueprintCallable, Category = "APC")
	bool IsDestroyed() const;

	UFUNCTION(BlueprintCallable, Category = "APC")
	int32 GetLoadedCrewCount() const;

	UFUNCTION(BlueprintCallable, Category = "APC|Crew")
	void SetEvacuationCrew(const TArray<AInfantryCharacter*>& InCrew);

	UFUNCTION(BlueprintCallable, Category = "APC|Crew")
	void SetBoardingMovePoint(AActor* InBoardingMovePoint);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Crew")
	bool bUseAutoBoardingMoveLocation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Crew")
	FVector AutoBoardingMoveLocalOffset = FVector(-400.0f, 0.0f, 0.0f);

	UFUNCTION(BlueprintCallable, Category = "APC|Crew")
	void OrderAssignedCrewToBoard();

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Crew")
	float BoardingDistanceCm = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Components")
	FName HitZoneComponentName = TEXT("APCHitZone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Components")
	FName BoardingZoneComponentName = TEXT("CrewBoardingZone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Components")
	FName GroundTraceFLName = TEXT("GroundTrace_FL");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Components")
	FName GroundTraceFRName = TEXT("GroundTrace_FR");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Components")
	FName GroundTraceRLName = TEXT("GroundTrace_RL");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Components")
	FName GroundTraceRRName = TEXT("GroundTrace_RR");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Components")
	FName DestroyedFXPointName = TEXT("DestroyedFXPoint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Components")
	FName NavObstacleComponentName = TEXT("APCNavObstacle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Movement")
	float MoveSpeedCmPerSec = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Movement")
	float TurnSpeedDegPerSec = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Movement")
	float AcceptanceRadiusCm = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Movement")
	float WheelAnimationSpeedScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Ground")
	float GroundTraceUpCm = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Ground")
	float GroundTraceDownCm = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Ground")
	float GroundClearanceCm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Ground")
	float GroundInterpSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Ground")
	float MaxGroundAlignSlopeDeg = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Animation")
	float OpenRearDoorsAngle = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Animation")
	float OpenHatchAngle = 95.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Animation")
	float DoorAnimSpeedDegPerSec = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Animation")
	float HatchAnimSpeedDegPerSec = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Crew")
	int32 ExpectedCrewCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Crew")
	float BoardingSecondsPerCrew = 5.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "APC|Crew")
	AActor* BoardingMovePoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Crew")
	FName BoardingMovePointName = TEXT("TP_APC_Boarding");


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Damage")
	bool bRequireBombArmedToDestroy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Damage")
	FName DroneWarheadComponentTag = TEXT("FPV_WarheadProbe");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Damage")
	UNiagaraSystem* DestroyedFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Damage")
	UMaterialInterface* DestroyedMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Damage")
	bool bApplyDestroyedMaterialToAllSlots = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Damage")
	TArray<int32> DestroyedMaterialElementIndices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|VFX")
	bool bUseTrackSmokeVFX = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|VFX")
	FString TrackSmokeComponentPrefix = TEXT("TrackSmoke");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|VFX")
	float TrackSmokeMinWheelSpeed = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Blueprint Events")
	FName SetLightsEmissivityEventName = TEXT("SetLightsEmissivity");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Debug")
	bool bDebugLogs = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Scenario")
	bool bAutoStartEvacuation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Scenario")
	float AutoStartDelaySeconds = 1.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "APC|Scenario")
	AActor* DefaultEvacPoint = nullptr;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "APC|Scenario")
	AActor* DefaultReturnPoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Scenario")
	FName DefaultEvacPointName = TEXT("TP_APC_Evac");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Scenario")
	FName DefaultReturnPointName = TEXT("TP_APC_Return");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Debug")
	bool bMovementDebugLogs = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "APC|Debug")
	float MovementDebugInterval = 0.25f;

private:

	float MovementDebugTimer = 0.0f;
	UPROPERTY()
	USkeletalMeshComponent* MeshComponent = nullptr;

	UPROPERTY()
	UAnimInstance* AnimInstance = nullptr;

	UPROPERTY()
	UPrimitiveComponent* HitZoneComponent = nullptr;

	UPROPERTY()
	UPrimitiveComponent* BoardingZoneComponent = nullptr;

	UPROPERTY()
	UPrimitiveComponent* NavObstacleComponent = nullptr;

	UPROPERTY()
	USceneComponent* GroundTraceFL = nullptr;

	UPROPERTY()
	USceneComponent* GroundTraceFR = nullptr;

	UPROPERTY()
	USceneComponent* GroundTraceRL = nullptr;

	UPROPERTY()
	USceneComponent* GroundTraceRR = nullptr;

	UPROPERTY()
	USceneComponent* DestroyedFXPoint = nullptr;

	UPROPERTY()
	AActor* EvacPoint = nullptr;

	UPROPERTY()
	AActor* ReturnPoint = nullptr;

	UPROPERTY()
	AInfantryCharacter* PendingCrewMember = nullptr;

	UPROPERTY()
	TArray<AInfantryCharacter*> AssignedCrew;

	bool bAssignedCrewOrderedToBoard = false;

	void CompactAssignedCrew();
	int32 GetRemainingAssignedCrewCount() const;
	bool ShouldFinishBoarding() const;

	UPROPERTY()
	TArray<UParticleSystemComponent*> TrackSmokeParticleComponents;

	UPROPERTY()
	TArray<UNiagaraComponent*> TrackSmokeNiagaraComponents;

	FTimerHandle AutoStartTimerHandle;
	FTimerHandle BoardingTimerHandle;

	EAPCAIState CurrentState = EAPCAIState::Inactive;

	int32 LoadedCrewCount = 0;

	float CurrentWheelSpeed = 0.0f;
	float CurrentRearDoorsAngle = 0.0f;
	float CurrentHatchAngle = 0.0f;

	float TargetRearDoorsAngle = 0.0f;
	float TargetHatchAngle = 0.0f;

	bool bDestroyed = false;
	bool bNavObstacleActive = false;
	void TryBoardOverlappingCrew();
	bool IsAssignedCrewMember(AActor* Actor) const;
	FVector GetBoardingMoveLocation() const;

	void ApplyDefaultAssetReferences();
	void AutoStartEvacuation();
	AActor* ResolveScenarioPoint(AActor* DirectActor, FName ActorName) const;

	void CacheComponents();
	void CacheTrackSmokeComponents();
	void BindOverlapEvents();
	void DisableLights();
	void SnapToGround();

	void SetState(EAPCAIState NewState);
	void TickState(float DeltaTime);
	void TickMoveToTarget(float DeltaTime, AActor* TargetActor, EAPCAIState ArrivedState);
	void TickDoorAnimation(float DeltaTime);
	void UpdateGroundAlignment(float DeltaTime);
	void UpdateAnimInstanceVariables();
	void UpdateTrackSmokeVFX();

	bool BuildGroundAlignedPose(const FVector& DesiredLocation, float DesiredYaw, float DeltaTime, FVector& OutLocation, FRotator& OutRotation) const;
	bool SampleGroundAtPose(const FVector& ActorLocation, const FRotator& ActorRotation, float& OutGroundZOffset, FVector& OutGroundNormal) const;

	void SetAnimFloat(FName VariableName, float Value);
	void SetWheelSpeed(float Value);
	void SetRearDoorsAngle(float Value);
	void SetHatchAngle(float Value);
	void SetTurretNeutral();

	void SetNavObstacleActive(bool bActive);
	void ApplyDestroyedVisuals(const FVector& HitLocation);

	void FinishBoardingCrew();
	void TryStartBoarding(AActor* OtherActor);

	bool IsDroneWarheadComponent(UPrimitiveComponent* OtherComp) const;
	bool IsDroneActor(AActor* OtherActor) const;
	bool IsDroneBombArmed(AActor* OtherActor) const;
	void ForceCrashDrone(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation);

	USceneComponent* FindSceneComponentByName(FName ComponentName) const;
	UPrimitiveComponent* FindPrimitiveComponentByName(FName ComponentName) const;

	bool IsTrackSmokeComponentName(const FString& ComponentName) const;

	void CallBlueprintEventNoParams(FName FunctionName);
	void CallBlueprintEventFloat(FName FunctionName, float Value);
	void CallBlueprintEventBool(FName FunctionName, bool bValue);

	void DebugLog(const FString& Message) const;

	UFUNCTION()
	void OnHitZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBoardingZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};