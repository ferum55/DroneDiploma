#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "InfantryAIController.generated.h"

class UBehaviorTree;

UENUM(BlueprintType)
enum class EInfantryMissionObjective : uint8
{
	None,
	HoldPosition,
	MoveToShelter,
	MoveToBoardingZone,
	MoveToObjective,
	MoveToEscapePoint
};

UCLASS()
class DIPLOMA_API AInfantryAIController : public AAIController
{
	GENERATED_BODY()

public:
	AInfantryAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UFUNCTION(BlueprintCallable)
	void SetTargetLocation(const FVector& Location);

	UFUNCTION(BlueprintCallable)
	void SetThreatActor(AActor* ThreatActor);

	UFUNCTION(BlueprintCallable)
	void SetHoldPosition(AActor* PositionActor);

	UFUNCTION(BlueprintCallable)
	void SetHoldPositionLocation(FVector PositionLocation);

	UFUNCTION(BlueprintCallable)
	void BeginMoveToShelter(AActor* ShelterPoint);

	UFUNCTION(BlueprintCallable)
	void BeginMoveToShelterLocation(FVector ShelterLocation);

	UFUNCTION(BlueprintCallable)
	void BeginMoveToBoardingZone(AActor* BoardingPoint);

	UFUNCTION(BlueprintCallable)
	void BeginMoveToBoardingZoneLocation(FVector BoardingLocation);

	UFUNCTION(BlueprintCallable)
	void BeginMissionObjectiveMoveTo(AActor* ObjectivePoint, bool bRunToObjective = true);

	UFUNCTION(BlueprintCallable)
	void BeginMissionObjectiveMoveToLocation(FVector ObjectiveLocation, bool bRunToObjective = true);

	UFUNCTION(BlueprintCallable)
	void BeginEscapeOnFootToLocation(FVector EscapeLocation);

	UFUNCTION(BlueprintCallable)
	void BeginEscapeOnFootToActor(AActor* EscapePoint);

	UFUNCTION(BlueprintCallable)
	void ResumeCurrentObjective();

	UFUNCTION(BlueprintCallable)
	void ClearMissionObjective();

	UFUNCTION(BlueprintCallable)
	bool IsWaitingAtMissionObjective() const;

	UFUNCTION(BlueprintCallable)
	EInfantryMissionObjective GetCurrentMissionObjective() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Mission Objective")
	float MissionObjectiveAcceptanceRadiusCm = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Mission Objective")
	float EscapeObjectiveAcceptanceRadiusCm = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Mission Objective")
	float BoardingObjectiveAcceptanceRadiusCm = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Mission Objective")
	float ShelterObjectiveAcceptanceRadiusCm = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Mission Objective")
	float HoldPositionAcceptanceRadiusCm = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Mission Objective")
	FName MissionObjectiveMoveState = TEXT("ReturnToPost");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Mission Objective")
	FName MissionObjectiveWaitState = TEXT("NoThreat");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName TargetLocationKey = TEXT("TargetLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName ThreatActorKey = TEXT("ThreatActor");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName HasThreatKey = TEXT("bHasThreat");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName ShouldFireKey = TEXT("bShouldFire");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName LastKnownThreatLocationKey = TEXT("LastKnownThreatLocation");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName AIStateKey = TEXT("AIState");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName ShouldRunKey = TEXT("bShouldRun");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName InCoverKey = TEXT("bInCover");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
	FName FireRelocatingKey = TEXT("bFireRelocating");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Navigation")
	int32 MaxNavInitAttempts = 20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Navigation")
	float NavInitRetryDelay = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	FName DroneActorTag = TEXT("PlayerDrone");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float VisualDetectionRangeCm = 7500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float AudioDetectionRangeCm = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float CloseThreatDistanceCm = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float EmergencyEvadeDistanceCm = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float SlowDroneSpeedKmh = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float FastApproachSpeedKmh = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float MaxFireElevationAngleDeg = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float ThreatUpdateInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float SightHorizontalFOVDeg = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Threat")
	float SightVerticalFOVDeg = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Debug")
	bool bDebugThreatDetection = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Debug")
	float ThreatDebugLogInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	float MovementDecisionCooldownSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	float DisperseMinDistanceCm = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	float DisperseMaxDistanceCm = 3400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	float EvadeSideDistanceCm = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	float BackOffMinDistanceCm = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	float BackOffMaxDistanceCm = 3800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	float MinAllySpacingCm = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	int32 MovementCandidateCount = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	float ReachedCombatMoveDistanceCm = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Movement")
	float MinNewMoveTargetDistanceCm = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Decision")
	float DecisionLockSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float FireEnterDelaySeconds = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float PrepareFireMinExitSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float FireMinExitSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float AIStartBurstIntervalSeconds = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float PanicBurstIntervalSeconds = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float PanicFireBaseSpreadDegrees = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float PanicFireMaxSpreadDegrees = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	int32 FireRelocateAfterMinBursts = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	int32 FireRelocateAfterMaxBursts = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float FireRelocationMinDistanceCm = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float FireRelocationMaxDistanceCm = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float FireRelocationMaxSeconds = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	int32 FireRelocationCandidateCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Search")
	float VisualSearchAfterDisperseDistanceCm = 6500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Search")
	float VisualSearchDurationSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Search")
	float VisualSearchCooldownSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Search")
	float VisualSearchTurnSpeedDegPerSecond = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Search")
	float ReturnToPostAfterNoThreatSeconds = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Facing")
	float CombatFacingInterpSpeed = 14.0f;

private:

	bool bEscapeOnFootReported = false;

	void ReportEscapeOnFootReached();

	float GetCurrentObjectiveAcceptanceRadius() const;

	bool bHasMissionObjective = false;
	bool bMissionObjectiveRun = true;
	bool bWaitingAtMissionObjective = false;

	EInfantryMissionObjective CurrentMissionObjective = EInfantryMissionObjective::None;

	TWeakObjectPtr<AActor> MissionObjectiveActor;
	FVector MissionObjectiveLocation = FVector::ZeroVector;

	void StartMissionObjective(EInfantryMissionObjective ObjectiveType, AActor* ObjectiveActor, FVector ObjectiveLocation, bool bRunToObjective);
	void ApplyMissionObjectiveMovement();
	bool HasReachedMissionObjective() const;

	FTimerHandle NavInitTimerHandle;
	FTimerHandle ThreatUpdateTimerHandle;

	int32 NavInitAttempts = 0;

	FVector HomeLocation = FVector::ZeroVector;
	FVector ReturnPostLocation = FVector::ZeroVector;
	FVector LastIssuedMoveTarget = FVector::ZeroVector;

	bool bHasReturnPostLocation = false;
	bool bHasActiveCombatMove = false;

	FName CurrentAIState = TEXT("None");
	FName LockedAIState = TEXT("None");

	float LockedAIStateUntilTime = -10000.0f;
	float LastMovementDecisionTime = -10000.0f;
	float LastThreatSeenTime = -10000.0f;
	float LastThreatDebugLogTime = -10000.0f;

	FName LastChosenDecisionState = TEXT("None");
	float DecisionLockedUntilTime = -10000.0f;

	bool bFireCandidateActive = false;
	bool bFireStateCommitted = false;
	float FireCandidateStartTime = -10000.0f;
	float FireStateEnterTime = -10000.0f;

	int32 FireBurstsBeforeRelocate = 1;
	int32 FireBurstsAtCurrentPosition = 0;
	bool bFireRelocating = false;
	float FireRelocationUntilTime = -10000.0f;

	float LastAIStartBurstTime = -10000.0f;
	float LastPanicBurstTime = -10000.0f;

	bool bVisualSearchActive = false;
	float VisualSearchUntilTime = -10000.0f;
	float LastVisualSearchStartTime = -10000.0f;
	float VisualSearchDirectionSign = 1.0f;

	bool bWaitAfterSearchActive = false;

	void TryInitializeAI();
	void StartThreatUpdates();
	void UpdateThreatState();
	void ClearThreatState();

	AActor* FindBestThreatActor() const;

	bool IsThreatDetected(AActor* ThreatActor, bool& bOutVisible, bool& bOutHeard, float& OutDistanceCm) const;
	bool HasLineOfSightToThreat(AActor* ThreatActor) const;
	bool IsThreatInsideSightCone(AActor* ThreatActor) const;

	float GetActorSpeedKmh(const AActor* Actor) const;
	float GetThreatClosingSpeedKmh(const AActor* ThreatActor) const;
	float GetThreatFacingDotToPawn(const AActor* ThreatActor) const;
	float GetElevationAngleToThreat(const AActor* ThreatActor) const;

	FName ChooseDesiredStateWeighted(bool bVisible, bool bHeard, float DistanceCm, float DroneSpeedKmh, float ClosingSpeedKmh, float DroneFacingDot, float ElevationAngleDeg) const;
	FName ChooseWeightedState(const TArray<TPair<FName, float>>& Options) const;

	FName ApplyFireStateTiming(FName DesiredAIState, bool bImmediateInterrupt);
	void ResetFireStateTiming();

	void ApplyMovementForState(FName AIState, AActor* ThreatActor);
	void StopCombatMovement();

	bool TryChooseDisperseLocation(AActor* ThreatActor, FVector& OutLocation) const;
	bool TryChooseSideEvadeLocation(AActor* ThreatActor, FVector& OutLocation) const;
	bool TryChooseBackOffLocation(AActor* ThreatActor, FVector& OutLocation) const;
	bool TryChooseFireRelocationLocation(AActor* ThreatActor, FVector& OutLocation) const;

	bool ProjectCandidateToNav(const FVector& RawCandidate, FVector& OutLocation) const;
	bool IsLocationFarEnoughFromAllies(const FVector& Location) const;
	bool HasReachedLastMoveTarget() const;
	bool IsNewMoveTargetUseful(const FVector& NewTargetLocation) const;

	void SetShouldRun(bool bShouldRun);
	void SetInCover(bool bInCover);

	void UpdateCombatFacing(FName AIState, AActor* ThreatActor);
	void RotatePawnTowardLocation(const FVector& TargetLocation);
	void UpdateVisualSearchFacing();

	void UpdateCombatFire(FName AIState, AActor* ThreatActor);
	void ResetCombatFire();
	void UpdatePanicFire(AActor* ThreatActor);
	float CalculatePanicFireSpreadDegrees(AActor* ThreatActor) const;

	void BeginFireRelocation();
	void UpdateFireRelocationState();
	void ResetFireRelocation();
	void SetFireRelocating(bool bRelocating);

	bool CanStartVisualSearch(float DistanceCm) const;
	void StartVisualSearch(float DistanceCm);
	void UpdateVisualSearchState(bool bVisible, bool bHeard, float DistanceCm);
	bool IsVisualSearchActive() const;
	bool IsWaitAfterSearchActive() const;
	bool ShouldReturnToPost() const;

	bool TryProjectPointToNavMesh(const FVector& RawLocation, FVector& OutProjectedLocation) const;
	void LogNavigationToTarget(const FVector& TargetLocation) const;
};