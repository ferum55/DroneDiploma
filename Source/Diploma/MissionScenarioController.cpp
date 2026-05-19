#include "MissionScenarioController.h"
#include "Engine/World.h"

AMissionScenarioController::AMissionScenarioController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMissionScenarioController::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateDefaultFlags)
	{
		ConfigureDefaultFlags();
	}

	if (bAutoStartMission)
	{
		StartMission();
	}
}

void AMissionScenarioController::StartMission()
{
	if (MissionState == EMissionScenarioState::Active)
	{
		return;
	}

	ResetMissionRuntimeState();

	MissionState = EMissionScenarioState::Active;

	if (GetWorld())
	{
		MissionStartTimeSeconds = GetWorld()->GetTimeSeconds();
	}

	BP_OnMissionStarted();

	UE_LOG(LogTemp, Warning, TEXT("[MISSION] Started | Type=%d Drones=%d"),
		static_cast<int32>(MissionType),
		AvailableDroneCount);
}

void AMissionScenarioController::RequestFinishMissionFromMenu()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	FinishMission(bPrimaryObjectiveCompleted);
}

void AMissionScenarioController::FinishMission(bool bSuccess)
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	MissionState = bSuccess ? EMissionScenarioState::Success : EMissionScenarioState::Failed;

	if (GetWorld())
	{
		MissionEndTimeSeconds = GetWorld()->GetTimeSeconds();
	}

	const float FinalTimeSeconds = GetElapsedTimeSeconds();

	BP_OnMissionFinished(bSuccess, Score, UsedDroneCount, FinalTimeSeconds);

	UE_LOG(LogTemp, Warning, TEXT("[MISSION] Finished | Success=%d Score=%d Drones=%d Time=%.1f"),
		bSuccess ? 1 : 0,
		Score,
		UsedDroneCount,
		FinalTimeSeconds);
}

void AMissionScenarioController::NotifyMissionObjectiveDestroyed(FName ObjectiveId, bool bPrimaryObjective, int32 ObjectiveScore, AActor* ObjectiveActor)
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	if (ObjectiveId == FriendlyPositionObjectiveId)
	{
		NotifyFriendlyPositionDestroyed();
		return;
	}

	if (ObjectiveId == MortarObjectiveId)
	{
		if (!bMortarDestroyed)
		{
			bMortarDestroyed = true;
			AddScoreOnce(TEXT("Objective_Mortar"), MortarScore);
			SetPrimaryObjectiveCompleted(TEXT("Primary_DestroyMortar"));
		}

		return;
	}

	if (ObjectiveId == AmmoCrateObjectiveId)
	{
		if (!bAmmoCrateDestroyed)
		{
			bAmmoCrateDestroyed = true;
			AddScoreOnce(TEXT("Objective_AmmoCrate"), AmmoCrateScore);
			CompleteFlag(TEXT("Bonus_DestroyAmmo"));
		}

		return;
	}

	AddScoreOnce(*FString::Printf(TEXT("Objective_%s"), *ObjectiveId.ToString()), ObjectiveScore);

	if (bPrimaryObjective)
	{
		SetPrimaryObjectiveCompleted(*FString::Printf(TEXT("Primary_%s"), *ObjectiveId.ToString()));
	}

	UE_LOG(LogTemp, Warning, TEXT("[MISSION] Objective destroyed | Id=%s Actor=%s Score=%d Total=%d"),
		*ObjectiveId.ToString(),
		*GetNameSafe(ObjectiveActor),
		ObjectiveScore,
		Score);
}

void AMissionScenarioController::NotifyInfantryKilled(FName KillGroupId, int32 Count)
{
	if (MissionState != EMissionScenarioState::Active || Count <= 0)
	{
		return;
	}

	if (KillGroupId == SPGCrewGroupId)
	{
		AwardGroupUnits(SPGCrewGroupId, Count, SPGCrewScorePerUnit, MaxSPGCrewCount, TEXT("Bonus_DestroySPGCrew"));
		return;
	}

	if (KillGroupId == TankCrewGroupId)
	{
		AwardGroupUnits(TankCrewGroupId, Count, TankCrewScorePerUnit, MaxTankCrewCount, TEXT("Bonus_DestroyTankCrew"));
		return;
	}

	AwardGroupUnits(EnemyInfantryGroupId, Count, EnemyInfantryScorePerUnit, MaxEnemyInfantryCount, TEXT("Bonus_DestroyInfantry"));
}

void AMissionScenarioController::NotifySPGMinorDamage()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	AddVehicleScoreProgress(TEXT("SPG_MinorDamage"), SPGVehicleScoreProgress, SPGMaxVehicleScore, SPGMinorDamageScore);
}

void AMissionScenarioController::NotifySPGImmobilized()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	AddVehicleScoreProgress(TEXT("SPG_Immobilized"), SPGVehicleScoreProgress, SPGMaxVehicleScore, SPGImmobilizedScore);
}

void AMissionScenarioController::NotifySPGDestroyed(int32 CrewInsideCount)
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	if (!bSPGDestroyed)
	{
		bSPGDestroyed = true;
		AddVehicleScoreProgress(TEXT("SPG_Destroyed"), SPGVehicleScoreProgress, SPGMaxVehicleScore, SPGMaxVehicleScore);
		SetPrimaryObjectiveCompleted(TEXT("Primary_DestroySPG"));

		if (CrewInsideCount > 0)
		{
			AwardGroupUnits(SPGCrewGroupId, CrewInsideCount, SPGCrewScorePerUnit, MaxSPGCrewCount, TEXT("Bonus_DestroySPGCrew"));
		}
	}
}

void AMissionScenarioController::NotifySPGEscaped()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	bSPGEscaped = true;
	FailFlag(TEXT("Primary_DestroySPG"));
	FinishMission(false);
}

void AMissionScenarioController::NotifyTankGunDestroyed()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	AddVehicleScoreProgress(TEXT("Tank_GunDestroyed"), TankVehicleScoreProgress, TankMaxVehicleScore, TankGunDestroyedScore);
	SetPrimaryObjectiveCompleted(TEXT("Primary_DefendFriendlyPosition"));
}

void AMissionScenarioController::NotifyTankImmobilized()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	AddVehicleScoreProgress(TEXT("Tank_Immobilized"), TankVehicleScoreProgress, TankMaxVehicleScore, TankImmobilizedScore);
	SetPrimaryObjectiveCompleted(TEXT("Primary_DefendFriendlyPosition"));
}

void AMissionScenarioController::NotifyTankCrewEvacuationStarted()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	SetPrimaryObjectiveCompleted(TEXT("Primary_DefendFriendlyPosition"));
}

void AMissionScenarioController::NotifyTankDestroyed(int32 CrewInsideCount)
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	if (!bTankDestroyed)
	{
		bTankDestroyed = true;
		AddVehicleScoreProgress(TEXT("Tank_Destroyed"), TankVehicleScoreProgress, TankMaxVehicleScore, TankMaxVehicleScore);
		SetPrimaryObjectiveCompleted(TEXT("Primary_DefendFriendlyPosition"));
		CompleteFlag(TEXT("Bonus_DestroyTank"));

		if (CrewInsideCount > 0)
		{
			AwardGroupUnits(TankCrewGroupId, CrewInsideCount, TankCrewScorePerUnit, MaxTankCrewCount, TEXT("Bonus_DestroyTankCrew"));
		}
	}
}

void AMissionScenarioController::NotifyAPCDestroyed(int32 LoadedCrewCount)
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	if (!bAPCDestroyed)
	{
		bAPCDestroyed = true;
		AddScoreOnce(TEXT("APC_Destroyed"), GetAPCDestroyedScoreForMission());
		CompleteFlag(TEXT("Bonus_DestroyAPC"));
	}

	if (LoadedCrewCount > 0)
	{
		if (MissionType == EMissionScenarioType::SPGStrike)
		{
			AwardGroupUnits(SPGCrewGroupId, LoadedCrewCount, SPGCrewScorePerUnit, MaxSPGCrewCount, TEXT("Bonus_DestroySPGCrew"));
		}
		else if (MissionType == EMissionScenarioType::TankAttack)
		{
			AwardGroupUnits(TankCrewGroupId, LoadedCrewCount, TankCrewScorePerUnit, MaxTankCrewCount, TEXT("Bonus_DestroyTankCrew"));
		}
	}
}

void AMissionScenarioController::NotifyFriendlyPositionDestroyed()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	bFriendlyPositionDestroyed = true;
	FailFlag(TEXT("Primary_DefendFriendlyPosition"));
	FinishMission(false);
}

void AMissionScenarioController::NotifyDroneUsed()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	UsedDroneCount = FMath::Clamp(UsedDroneCount + 1, 0, AvailableDroneCount);
	EvaluateAfterDroneUsed();

	UE_LOG(LogTemp, Warning, TEXT("[MISSION] Drone used | Used=%d Available=%d"),
		UsedDroneCount,
		AvailableDroneCount);
}

int32 AMissionScenarioController::GetScore() const
{
	return Score;
}

int32 AMissionScenarioController::GetMaxScore() const
{
	return MaxMissionScore;
}

int32 AMissionScenarioController::GetUsedDroneCount() const
{
	return UsedDroneCount;
}

int32 AMissionScenarioController::GetRemainingDroneCount() const
{
	return FMath::Max(0, AvailableDroneCount - UsedDroneCount);
}

bool AMissionScenarioController::IsPrimaryObjectiveCompleted() const
{
	return bPrimaryObjectiveCompleted;
}

bool AMissionScenarioController::CanPlayerFinishMissionSuccessfully() const
{
	return MissionState == EMissionScenarioState::Active && bPrimaryObjectiveCompleted;
}

bool AMissionScenarioController::IsMissionFinished() const
{
	return MissionState == EMissionScenarioState::Success || MissionState == EMissionScenarioState::Failed;
}

EMissionScenarioState AMissionScenarioController::GetMissionState() const
{
	return MissionState;
}

float AMissionScenarioController::GetElapsedTimeSeconds() const
{
	if (!GetWorld())
	{
		return 0.0f;
	}

	if (IsMissionFinished())
	{
		return FMath::Max(0.0f, MissionEndTimeSeconds - MissionStartTimeSeconds);
	}

	return FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - MissionStartTimeSeconds);
}


const TArray<FMissionFlagState>& AMissionScenarioController::GetMissionFlags() const
{
	return MissionFlags;
}

void AMissionScenarioController::ResetMissionRuntimeState()
{
	Score = 0;
	UsedDroneCount = 0;
	MissionStartTimeSeconds = 0.0f;
	MissionEndTimeSeconds = 0.0f;
	bPrimaryObjectiveCompleted = false;

	SPGVehicleScoreProgress = 0;
	TankVehicleScoreProgress = 0;

	bMortarDestroyed = false;
	bAmmoCrateDestroyed = false;
	bFriendlyPositionDestroyed = false;
	bSPGEscaped = false;
	bSPGDestroyed = false;
	bTankDestroyed = false;
	bAPCDestroyed = false;

	AwardedScoreEvents.Empty();
	CountedUnitsByGroup.Empty();

	if (bGenerateDefaultFlags)
	{
		ConfigureDefaultFlags();
	}

	for (FMissionFlagState& Flag : MissionFlags)
	{
		Flag.bCompleted = false;
		Flag.bFailed = false;
	}
}

void AMissionScenarioController::ConfigureDefaultFlags()
{
	MissionFlags.Empty();

	if (MissionType == EMissionScenarioType::MortarPosition)
	{
		AddFlag(TEXT("Primary_DestroyMortar"), TEXT("Знищити мінометну позицію"), true);
		AddFlag(TEXT("Bonus_DestroyAmmo"), TEXT("Знищити боєкомплект"), false);
		AddFlag(TEXT("Bonus_DestroyInfantry"), TEXT("Знищити піхоту на позиції"), false);
	}
	else if (MissionType == EMissionScenarioType::SPGStrike)
	{
		AddFlag(TEXT("Primary_DestroySPG"), TEXT("Знищити САУ"), true);
		AddFlag(TEXT("Bonus_DestroySPGCrew"), TEXT("Знищити екіпаж"), false);
		AddFlag(TEXT("Bonus_DestroyAPC"), TEXT("Знищити евакуаційну машину"), false);
	}
	else if (MissionType == EMissionScenarioType::TankAttack)
	{
		AddFlag(TEXT("Primary_DefendFriendlyPosition"), TEXT("Не допустити знищення позиції"), true);
		AddFlag(TEXT("Bonus_DestroyTank"), TEXT("Знищити танк Т-72Б3"), false);
		AddFlag(TEXT("Bonus_DestroyTankCrew"), TEXT("Знищити екіпаж"), false);
		AddFlag(TEXT("Bonus_DestroyAPC"), TEXT("Знищити евакуаційну машину"), false);
	}
}

void AMissionScenarioController::AddFlag(FName FlagId, const FString& DisplayName, bool bPrimary)
{
	FMissionFlagState Flag;
	Flag.FlagId = FlagId;
	Flag.DisplayName = FText::FromString(DisplayName);
	Flag.bPrimary = bPrimary;
	Flag.bCompleted = false;
	Flag.bFailed = false;

	MissionFlags.Add(Flag);
}

void AMissionScenarioController::AddScoreOnce(FName EventId, int32 ScoreDelta)
{
	if (EventId.IsNone() || ScoreDelta <= 0 || AwardedScoreEvents.Contains(EventId))
	{
		return;
	}

	AwardedScoreEvents.Add(EventId);
	AddScoreDirect(EventId, ScoreDelta);
}

void AMissionScenarioController::AddScoreDirect(FName EventId, int32 ScoreDelta)
{
	if (ScoreDelta <= 0)
	{
		return;
	}

	const int32 OldScore = Score;
	Score = FMath::Clamp(Score + ScoreDelta, 0, MaxMissionScore);
	const int32 ActualDelta = Score - OldScore;

	if (ActualDelta <= 0)
	{
		return;
	}

	BP_OnScoreChanged(Score);

	UE_LOG(LogTemp, Warning, TEXT("[MISSION SCORE] Event=%s Delta=%d Total=%d"),
		*EventId.ToString(),
		ActualDelta,
		Score);
}

void AMissionScenarioController::AddVehicleScoreProgress(FName EventId, int32& ProgressValue, int32 MaxVehicleScore, int32 ScoreDelta)
{
	if (EventId.IsNone() || ScoreDelta <= 0 || AwardedScoreEvents.Contains(EventId))
	{
		return;
	}

	AwardedScoreEvents.Add(EventId);

	const int32 OldProgress = ProgressValue;
	ProgressValue = FMath::Clamp(ProgressValue + ScoreDelta, 0, MaxVehicleScore);
	const int32 ActualDelta = ProgressValue - OldProgress;

	AddScoreDirect(EventId, ActualDelta);
}

void AMissionScenarioController::SetPrimaryObjectiveCompleted(FName PrimaryFlagId)
{
	if (!bPrimaryObjectiveCompleted)
	{
		bPrimaryObjectiveCompleted = true;
		BP_OnPrimaryObjectiveCompleted();
	}

	CompleteFlag(PrimaryFlagId);
}

void AMissionScenarioController::CompleteFlag(FName FlagId)
{
	FMissionFlagState* Flag = FindFlag(FlagId);

	if (!Flag || Flag->bCompleted)
	{
		return;
	}

	Flag->bCompleted = true;
	Flag->bFailed = false;

	BP_OnFlagChanged(FlagId, true, false);

	UE_LOG(LogTemp, Warning, TEXT("[MISSION FLAG] Completed=%s"), *FlagId.ToString());
}

void AMissionScenarioController::FailFlag(FName FlagId)
{
	FMissionFlagState* Flag = FindFlag(FlagId);

	if (!Flag || Flag->bFailed)
	{
		return;
	}

	Flag->bFailed = true;
	Flag->bCompleted = false;

	BP_OnFlagChanged(FlagId, false, true);

	UE_LOG(LogTemp, Warning, TEXT("[MISSION FLAG] Failed=%s"), *FlagId.ToString());
}

FMissionFlagState* AMissionScenarioController::FindFlag(FName FlagId)
{
	for (FMissionFlagState& Flag : MissionFlags)
	{
		if (Flag.FlagId == FlagId)
		{
			return &Flag;
		}
	}

	return nullptr;
}

void AMissionScenarioController::AwardGroupUnits(FName GroupId, int32 Count, int32 ScorePerUnit, int32 MaxCount, FName CompletionFlagId)
{
	if (GroupId.IsNone() || Count <= 0 || ScorePerUnit <= 0 || MaxCount <= 0)
	{
		return;
	}

	const int32 AlreadyCounted = CountedUnitsByGroup.FindRef(GroupId);
	const int32 Remaining = FMath::Max(0, MaxCount - AlreadyCounted);
	const int32 NewCount = FMath::Min(Count, Remaining);

	if (NewCount <= 0)
	{
		return;
	}

	CountedUnitsByGroup.FindOrAdd(GroupId) = AlreadyCounted + NewCount;

	AddScoreDirect(*FString::Printf(TEXT("Group_%s_%d"), *GroupId.ToString(), AlreadyCounted + NewCount), NewCount * ScorePerUnit);

	if (CountedUnitsByGroup.FindRef(GroupId) >= MaxCount)
	{
		CompleteFlag(CompletionFlagId);
	}
}

void AMissionScenarioController::EvaluateAfterDroneUsed()
{
	if (UsedDroneCount < AvailableDroneCount)
	{
		return;
	}

	FinishMission(bPrimaryObjectiveCompleted);
}

int32 AMissionScenarioController::GetAPCDestroyedScoreForMission() const
{
	if (MissionType == EMissionScenarioType::SPGStrike)
	{
		return SPGAPCDestroyedScore;
	}

	if (MissionType == EMissionScenarioType::TankAttack)
	{
		return TankAPCDestroyedScore;
	}

	return 0;
}

FName AMissionScenarioController::MakeEventId(const FString& Prefix) const
{
	return FName(*Prefix);
}

void AMissionScenarioController::DebugStartMission()
{
	StartMission();
}

void AMissionScenarioController::DebugUseDrone()
{
	NotifyDroneUsed();
}

void AMissionScenarioController::DebugFinishFromMenu()
{
	RequestFinishMissionFromMenu();
}

void AMissionScenarioController::DebugMortarDestroyed()
{
	NotifyMissionObjectiveDestroyed(MortarObjectiveId, true, MortarScore, nullptr);
}

void AMissionScenarioController::DebugAmmoDestroyed()
{
	NotifyMissionObjectiveDestroyed(AmmoCrateObjectiveId, false, AmmoCrateScore, nullptr);
}

void AMissionScenarioController::DebugInfantryKilled()
{
	NotifyInfantryKilled(EnemyInfantryGroupId, 1);
}

void AMissionScenarioController::DebugSPGMinorDamage()
{
	NotifySPGMinorDamage();
}

void AMissionScenarioController::DebugSPGImmobilized()
{
	NotifySPGImmobilized();
}

void AMissionScenarioController::DebugSPGDestroyed()
{
	NotifySPGDestroyed(MaxSPGCrewCount);
}

void AMissionScenarioController::DebugSPGEscaped()
{
	NotifySPGEscaped();
}

void AMissionScenarioController::DebugTankGunDestroyed()
{
	NotifyTankGunDestroyed();
}

void AMissionScenarioController::DebugTankImmobilized()
{
	NotifyTankImmobilized();
}

void AMissionScenarioController::DebugTankDestroyed()
{
	NotifyTankDestroyed(MaxTankCrewCount);
}

void AMissionScenarioController::DebugAPCDestroyed()
{
	if (MissionType == EMissionScenarioType::SPGStrike)
	{
		NotifyAPCDestroyed(MaxSPGCrewCount);
		return;
	}

	if (MissionType == EMissionScenarioType::TankAttack)
	{
		NotifyAPCDestroyed(MaxTankCrewCount);
		return;
	}

	NotifyAPCDestroyed(0);
}

void AMissionScenarioController::DebugFriendlyPositionDestroyed()
{
	NotifyFriendlyPositionDestroyed();
}

int32 AMissionScenarioController::GetAvailableDroneCount() const
{
	return AvailableDroneCount;
}

EMissionScenarioType AMissionScenarioController::GetMissionType() const
{
	return MissionType;
}

FText AMissionScenarioController::GetMissionTitle() const
{
	if (MissionType == EMissionScenarioType::MortarPosition)
	{
		return FText::FromString(TEXT("ЗНИЩЕННЯ МІНОМЕТНОЇ ПОЗИЦІЇ"));
	}

	if (MissionType == EMissionScenarioType::SPGStrike)
	{
		return FText::FromString(TEXT("ЗНИЩЕННЯ САУ"));
	}

	if (MissionType == EMissionScenarioType::TankAttack)
	{
		return FText::FromString(TEXT("ЗАХИСТ ДРУЖНЬОЇ ПОЗИЦІЇ ВІД ТАНКА"));
	}

	return FText::FromString(TEXT("РЕЗУЛЬТАТИ МІСІЇ"));
}