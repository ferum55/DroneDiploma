#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissionScenarioController.generated.h"

UENUM(BlueprintType)
enum class EMissionScenarioType : uint8
{
	MortarPosition,
	SPGStrike,
	TankAttack
};

UENUM(BlueprintType)
enum class EMissionScenarioState : uint8
{
	NotStarted,
	Active,
	Success,
	Failed
};

USTRUCT(BlueprintType)
struct FMissionFlagState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName FlagId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPrimary = false;

	UPROPERTY(BlueprintReadOnly)
	bool bCompleted = false;

	UPROPERTY(BlueprintReadOnly)
	bool bFailed = false;
};

UCLASS()
class DIPLOMA_API AMissionScenarioController : public AActor
{
	GENERATED_BODY()

public:
	AMissionScenarioController();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	void StartMission();

	UFUNCTION(BlueprintCallable, Category = "Mission")
	void RequestFinishMissionFromMenu();

	UFUNCTION(BlueprintCallable, Category = "Mission")
	void FinishMission(bool bSuccess);

	UFUNCTION(BlueprintCallable, Category = "Mission|Objectives")
	void NotifyMissionObjectiveDestroyed(FName ObjectiveId, bool bPrimaryObjective, int32 ObjectiveScore, AActor* ObjectiveActor);

	UFUNCTION(BlueprintCallable, Category = "Mission|Infantry")
	void NotifyInfantryKilled(FName KillGroupId, int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Mission|SPG")
	void NotifySPGMinorDamage();

	UFUNCTION(BlueprintCallable, Category = "Mission|SPG")
	void NotifySPGImmobilized();

	UFUNCTION(BlueprintCallable, Category = "Mission|SPG")
	void NotifySPGDestroyed(int32 CrewInsideCount);

	UFUNCTION(BlueprintCallable, Category = "Mission|SPG")
	void NotifySPGEscaped();

	UFUNCTION(BlueprintCallable, Category = "Mission|Tank")
	void NotifyTankGunDestroyed();

	UFUNCTION(BlueprintCallable, Category = "Mission|Tank")
	void NotifyTankImmobilized();

	UFUNCTION(BlueprintCallable, Category = "Mission|Tank")
	void NotifyTankCrewEvacuationStarted();

	UFUNCTION(BlueprintCallable, Category = "Mission|Tank")
	void NotifyTankDestroyed(int32 CrewInsideCount);

	UFUNCTION(BlueprintCallable, Category = "Mission|APC")
	void NotifyAPCDestroyed(int32 LoadedCrewCount);

	UFUNCTION(BlueprintCallable, Category = "Mission|Friendly")
	void NotifyFriendlyPositionDestroyed();

	UFUNCTION(BlueprintCallable, Category = "Mission|Drone")
	void NotifyDroneUsed();

	UFUNCTION(BlueprintCallable, Category = "Mission")
	int32 GetScore() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	int32 GetMaxScore() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	int32 GetUsedDroneCount() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	int32 GetRemainingDroneCount() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool IsPrimaryObjectiveCompleted() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool CanPlayerFinishMissionSuccessfully() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	bool IsMissionFinished() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	EMissionScenarioState GetMissionState() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	float GetElapsedTimeSeconds() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	const TArray<FMissionFlagState>& GetMissionFlags() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	int32 GetAvailableDroneCount() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	EMissionScenarioType GetMissionType() const;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	FText GetMissionTitle() const;

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugStartMission();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugUseDrone();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugFinishFromMenu();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugMortarDestroyed();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugAmmoDestroyed();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugInfantryKilled();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugSPGMinorDamage();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugSPGImmobilized();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugSPGDestroyed();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugSPGEscaped();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugTankGunDestroyed();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugTankImmobilized();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugTankDestroyed();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugAPCDestroyed();

	UFUNCTION(CallInEditor, Category = "Mission|Debug")
	void DebugFriendlyPositionDestroyed();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	EMissionScenarioType MissionType = EMissionScenarioType::MortarPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	bool bAutoStartMission = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	bool bGenerateDefaultFlags = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	int32 AvailableDroneCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score")
	int32 MaxMissionScore = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Objective Ids")
	FName MortarObjectiveId = TEXT("Mortar");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Objective Ids")
	FName AmmoCrateObjectiveId = TEXT("AmmoCrate");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Objective Ids")
	FName FriendlyPositionObjectiveId = TEXT("FriendlyPosition");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Groups")
	FName EnemyInfantryGroupId = TEXT("EnemyInfantry");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Groups")
	FName SPGCrewGroupId = TEXT("SPGCrew");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Groups")
	FName TankCrewGroupId = TEXT("TankCrew");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Mortar")
	int32 MortarScore = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Mortar")
	int32 AmmoCrateScore = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Mortar")
	int32 EnemyInfantryScorePerUnit = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Mortar")
	int32 MaxEnemyInfantryCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|SPG")
	int32 SPGMaxVehicleScore = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|SPG")
	int32 SPGMinorDamageScore = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|SPG")
	int32 SPGImmobilizedScore = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|SPG")
	int32 SPGCrewScorePerUnit = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|SPG")
	int32 MaxSPGCrewCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|SPG")
	int32 SPGAPCDestroyedScore = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Tank")
	int32 TankMaxVehicleScore = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Tank")
	int32 TankGunDestroyedScore = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Tank")
	int32 TankImmobilizedScore = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Tank")
	int32 TankCrewScorePerUnit = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Tank")
	int32 MaxTankCrewCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Score|Tank")
	int32 TankAPCDestroyedScore = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Flags")
	TArray<FMissionFlagState> MissionFlags;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	EMissionScenarioState MissionState = EMissionScenarioState::NotStarted;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	int32 UsedDroneCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bPrimaryObjectiveCompleted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	float MissionStartTimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	float MissionEndTimeSeconds = 0.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void BP_OnMissionStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void BP_OnMissionFinished(bool bSuccess, int32 FinalScore, int32 FinalUsedDrones, float FinalTimeSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void BP_OnScoreChanged(int32 NewScore);

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void BP_OnFlagChanged(FName FlagId, bool bCompleted, bool bFailed);

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void BP_OnPrimaryObjectiveCompleted();

private:
	TSet<FName> AwardedScoreEvents;
	TMap<FName, int32> CountedUnitsByGroup;

	int32 SPGVehicleScoreProgress = 0;
	int32 TankVehicleScoreProgress = 0;

	bool bMortarDestroyed = false;
	bool bAmmoCrateDestroyed = false;
	bool bFriendlyPositionDestroyed = false;
	bool bSPGEscaped = false;
	bool bSPGDestroyed = false;
	bool bTankDestroyed = false;
	bool bAPCDestroyed = false;

	void ResetMissionRuntimeState();
	void ConfigureDefaultFlags();
	void AddFlag(FName FlagId, const FString& DisplayName, bool bPrimary);
	void AddScoreOnce(FName EventId, int32 ScoreDelta);
	void AddScoreDirect(FName EventId, int32 ScoreDelta);
	void AddVehicleScoreProgress(FName EventId, int32& ProgressValue, int32 MaxVehicleScore, int32 ScoreDelta);
	void SetPrimaryObjectiveCompleted(FName PrimaryFlagId);
	void CompleteFlag(FName FlagId);
	void FailFlag(FName FlagId);
	FMissionFlagState* FindFlag(FName FlagId);
	void AwardGroupUnits(FName GroupId, int32 Count, int32 ScorePerUnit, int32 MaxCount, FName CompletionFlagId);
	void EvaluateAfterDroneUsed();
	int32 GetAPCDestroyedScoreForMission() const;
	FName MakeEventId(const FString& Prefix) const;
};