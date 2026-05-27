#include "MissionScenarioController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

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

	if (bPrimaryObjectiveCompleted)
	{
		ShowMissionEventOnScreen(TEXT("Завершення місії з меню"), FColor::Yellow);
	}
	else
	{
		ShowMissionEventOnScreen(TEXT("Головну ціль не виконано"), FColor::Red);
	}

	FinishMission(bPrimaryObjectiveCompleted);
}
void AMissionScenarioController::FinishMission(bool bSuccess)
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	MissionFinishReason = bSuccess
		? TEXT("МІСІЮ ВИКОНАНО")
		: TEXT("МІСІЮ ПРОВАЛЕНО");

	MissionState = bSuccess ? EMissionScenarioState::Success : EMissionScenarioState::Failed;

	if (GetWorld())
	{
		MissionEndTimeSeconds = GetWorld()->GetTimeSeconds();
	}

	const float FinalTimeSeconds = GetElapsedTimeSeconds();

	ShowMissionFinishReasonOnScreen(bSuccess);

	BP_OnMissionFinished(bSuccess, Score, UsedDroneCount, FinalTimeSeconds);

	UE_LOG(LogTemp, Warning, TEXT("[MISSION] Finished | Success=%d Score=%d Drones=%d Time=%.1f Result=%s"),
		bSuccess ? 1 : 0,
		Score,
		UsedDroneCount,
		FinalTimeSeconds,
		*MissionFinishReason);
}

void AMissionScenarioController::SetMissionFinishReason(const FString& Reason)
{
	if (!MissionFinishReason.IsEmpty())
	{
		return;
	}

	MissionFinishReason = Reason;
}

void AMissionScenarioController::ShowMissionFinishReasonOnScreen(bool bSuccess) const
{
	if (!GEngine)
	{
		return;
	}

	const FColor MessageColor = bSuccess ? FColor::Green : FColor::Red;

	GEngine->AddOnScreenDebugMessage(
		-1,
		5.0f,
		MessageColor,
		MissionFinishReason
	);
}

void AMissionScenarioController::ShowMissionEventOnScreen(const FString& Message, const FColor& Color) const
{
	if (!GEngine)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(
		-1,
		4.0f,
		Color,
		Message
	);
}

FString AMissionScenarioController::GetMissionFinishReason() const
{
	return MissionFinishReason;
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

		EvaluateMissionCompletion();
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

		EvaluateMissionCompletion();
		return;
	}

	AddScoreOnce(*FString::Printf(TEXT("Objective_%s"), *ObjectiveId.ToString()), ObjectiveScore);

	if (bPrimaryObjective)
	{
		SetPrimaryObjectiveCompleted(*FString::Printf(TEXT("Primary_%s"), *ObjectiveId.ToString()));
	}

	EvaluateMissionCompletion();

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
		EvaluateMissionCompletion();
		return;
	}

	if (KillGroupId == TankCrewGroupId)
	{
		AwardGroupUnits(TankCrewGroupId, Count, TankCrewScorePerUnit, MaxTankCrewCount, TEXT("Bonus_DestroyTankCrew"));
		EvaluateMissionCompletion();
		return;
	}

	AwardGroupUnits(EnemyInfantryGroupId, Count, EnemyInfantryScorePerUnit, MaxEnemyInfantryCount, TEXT("Bonus_DestroyInfantry"));
	EvaluateMissionCompletion();
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

	bSPGNeutralized = true;
	ShowMissionEventOnScreen(TEXT("САУ знерухомлено"), FColor::Yellow);

	AddVehicleScoreProgress(TEXT("SPG_Immobilized"), SPGVehicleScoreProgress, SPGMaxVehicleScore, SPGImmobilizedScore);
	SetPrimaryObjectiveCompleted(TEXT("Primary_DestroySPG"));

	EvaluateMissionCompletion();
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
		bSPGNeutralized = true;

		AddVehicleScoreProgress(TEXT("SPG_Destroyed"), SPGVehicleScoreProgress, SPGMaxVehicleScore, SPGMaxVehicleScore);
		SetPrimaryObjectiveCompleted(TEXT("Primary_DestroySPG"));

		if (CrewInsideCount > 0)
		{
			AwardGroupUnits(SPGCrewGroupId, CrewInsideCount, SPGCrewScorePerUnit, MaxSPGCrewCount, TEXT("Bonus_DestroySPGCrew"));
		}

		if (CrewInsideCount > 0)
		{
			ShowMissionEventOnScreen(TEXT("САУ знищено разом з екіпажем"), FColor::Green);
		}
		else
		{
			ShowMissionEventOnScreen(TEXT("САУ знищено"), FColor::Green);
		}
	}

	EvaluateMissionCompletion();
}

void AMissionScenarioController::NotifySPGEscaped()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	bSPGEscaped = true;
	FailFlag(TEXT("Primary_DestroySPG"));
	ShowMissionEventOnScreen(TEXT("САУ залишила район бойового завдання"), FColor::Red);

	EvaluateMissionCompletion();
}

void AMissionScenarioController::NotifyTankGunDestroyed()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}
	ShowMissionEventOnScreen(TEXT("Ствол танка пошкоджено"), FColor::Yellow);
	AddVehicleScoreProgress(TEXT("Tank_GunDestroyed"), TankVehicleScoreProgress, TankMaxVehicleScore, TankGunDestroyedScore);
	SetPrimaryObjectiveCompleted(TEXT("Primary_DefendFriendlyPosition"));

	EvaluateMissionCompletion();
}

void AMissionScenarioController::NotifyTankImmobilized()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	bTankNeutralized = true;
	ShowMissionEventOnScreen(TEXT("Танк знерухомлено"), FColor::Yellow);

	AddVehicleScoreProgress(TEXT("Tank_Immobilized"), TankVehicleScoreProgress, TankMaxVehicleScore, TankImmobilizedScore);
	SetPrimaryObjectiveCompleted(TEXT("Primary_DefendFriendlyPosition"));

	EvaluateMissionCompletion();
}

void AMissionScenarioController::NotifyTankCrewEvacuationStarted()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	bTankNeutralized = true;
	ShowMissionEventOnScreen(TEXT("Екіпаж танка почав евакуацію"), FColor::Yellow);

	SetPrimaryObjectiveCompleted(TEXT("Primary_DefendFriendlyPosition"));

	EvaluateMissionCompletion();
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
		bTankNeutralized = true;
		if (CrewInsideCount > 0)
		{
			ShowMissionEventOnScreen(TEXT("Танк знищено разом з екіпажем"), FColor::Green);
		}
		else
		{
			ShowMissionEventOnScreen(TEXT("Танк знищено"), FColor::Green);
		}

		AddVehicleScoreProgress(TEXT("Tank_Destroyed"), TankVehicleScoreProgress, TankMaxVehicleScore, TankMaxVehicleScore);
		SetPrimaryObjectiveCompleted(TEXT("Primary_DefendFriendlyPosition"));
		CompleteFlag(TEXT("Bonus_DestroyTank"));

		if (CrewInsideCount > 0)
		{
			AwardGroupUnits(TankCrewGroupId, CrewInsideCount, TankCrewScorePerUnit, MaxTankCrewCount, TEXT("Bonus_DestroyTankCrew"));
		}
	}

	EvaluateMissionCompletion();
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
		if (LoadedCrewCount > 0)
		{
			ShowMissionEventOnScreen(TEXT("Евакуаційну машину знищено разом з екіпажем"), FColor::Green);
		}
		else
		{
			ShowMissionEventOnScreen(TEXT("Евакуаційну машину знищено"), FColor::Green);
		}
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

	EvaluateMissionCompletion();
}

void AMissionScenarioController::NotifyAPCEscaped(int32 LoadedCrewCount)
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	if (!bAPCEscaped)
	{
		bAPCEscaped = true;
		ShowMissionEventOnScreen(TEXT("Евакуаційна машина залишила район"), FColor::Red);
		FailFlag(TEXT("Bonus_DestroyAPC"));
	}

	if (LoadedCrewCount > 0)
	{
		const int32 ExpectedCrew = GetExpectedCrewCountForMission();
		const int32 DestroyedCrew = GetDestroyedCrewCountForMission();
		const int32 RemainingCrewToResolve = FMath::Max(0, ExpectedCrew - DestroyedCrew - EscapedCrewCount);
		const int32 NewlyEscapedCrew = FMath::Min(LoadedCrewCount, RemainingCrewToResolve);

		EscapedCrewCount += NewlyEscapedCrew;
		ShowMissionEventOnScreen(
			"Член екіпажу втік пішки"
		);

		const FName CrewFlagId = GetCrewBonusFlagIdForMission();

		if (!CrewFlagId.IsNone())
		{
			FailFlag(CrewFlagId);
		}
	}

	EvaluateMissionCompletion();

	UE_LOG(LogTemp, Warning, TEXT("[MISSION] APC escaped | LoadedCrew=%d EscapedCrewTotal=%d"),
		LoadedCrewCount,
		EscapedCrewCount);
}

void AMissionScenarioController::NotifyCrewEscapedOnFoot(int32 InEscapedCrewCount)
{
	if (MissionState != EMissionScenarioState::Active || InEscapedCrewCount <= 0)
	{
		return;
	}

	const int32 ExpectedCrew = GetExpectedCrewCountForMission();
	const int32 DestroyedCrew = GetDestroyedCrewCountForMission();
	const int32 RemainingCrewToResolve = FMath::Max(0, ExpectedCrew - DestroyedCrew - EscapedCrewCount);
	const int32 NewlyEscapedCrew = FMath::Min(InEscapedCrewCount, RemainingCrewToResolve);

	if (NewlyEscapedCrew <= 0)
	{
		return;
	}

	EscapedCrewCount += NewlyEscapedCrew;

	const FName CrewFlagId = GetCrewBonusFlagIdForMission();

	if (!CrewFlagId.IsNone())
	{
		FailFlag(CrewFlagId);
	}

	EvaluateMissionCompletion();

	UE_LOG(LogTemp, Warning, TEXT("[MISSION] Crew escaped on foot | Count=%d EscapedCrewTotal=%d"),
		NewlyEscapedCrew,
		EscapedCrewCount);
}

void AMissionScenarioController::NotifyFriendlyPositionDestroyed()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	bFriendlyPositionDestroyed = true;
	ShowMissionEventOnScreen(TEXT("Дружню позицію знищено"), FColor::Red);
	FailFlag(TEXT("Primary_DefendFriendlyPosition"));

	EvaluateMissionCompletion();
}

void AMissionScenarioController::NotifyDroneUsed()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	UsedDroneCount = FMath::Clamp(UsedDroneCount + 1, 0, AvailableDroneCount);

	if (!bDroneUseEvaluationPending)
	{
		bDroneUseEvaluationPending = true;

		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AMissionScenarioController::EvaluateAfterDroneUsed);
		}
		else
		{
			EvaluateAfterDroneUsed();
		}
	}

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
	bDroneUseEvaluationPending = false;
	MissionStartTimeSeconds = 0.0f;
	MissionEndTimeSeconds = 0.0f;
	bPrimaryObjectiveCompleted = false;
	MissionFinishReason.Empty();

	SPGVehicleScoreProgress = 0;
	TankVehicleScoreProgress = 0;

	bMortarDestroyed = false;
	bAmmoCrateDestroyed = false;
	bFriendlyPositionDestroyed = false;
	bSPGEscaped = false;
	bSPGDestroyed = false;
	bTankDestroyed = false;
	bAPCDestroyed = false;

	bSPGEscaped = false;
	bSPGDestroyed = false;
	bTankDestroyed = false;
	bAPCDestroyed = false;
	bSPGNeutralized = false;
	bTankNeutralized = false;
	bAPCEscaped = false;

	EscapedCrewCount = 0;
	bAPCEscaped = false;
	EscapedCrewCount = 0;

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
		AddFlag(TEXT("Primary_DestroyMortar"), TEXT("ЗНИЩИТИ МІНОМЕТНУ ПОЗИЦІЮ"), true);
		AddFlag(TEXT("Bonus_DestroyAmmo"), TEXT("ЗНИЩИТИ БОЄКОМПЛЕКТ"), false);
		AddFlag(TEXT("Bonus_DestroyInfantry"), TEXT("ЗНИЩИТИ ПІХОТУ НА ПОЗИЦІЇ"), false);
	}
	else if (MissionType == EMissionScenarioType::SPGStrike)
	{
		AddFlag(TEXT("Primary_DestroySPG"), TEXT("ЗНИЩИТИ САУ"), true);
		AddFlag(TEXT("Bonus_DestroySPGCrew"), TEXT("ЗНИЩИТИ ЕКІПАЖ"), false);
		AddFlag(TEXT("Bonus_DestroyAPC"), TEXT("ЗНИЩИТИ ЕВАКУАЦІЙНУ МАШИНУ"), false);
	}
	else if (MissionType == EMissionScenarioType::TankAttack)
	{
		AddFlag(TEXT("Primary_DefendFriendlyPosition"), TEXT("НЕ ДОПУСТИТИ ЗНИЩЕННЯ ПОЗИЦІЇ"), true);
		AddFlag(TEXT("Bonus_DestroyTank"), TEXT("ЗНИЩИТИ ТАНК Т-72Б3"), false);
		AddFlag(TEXT("Bonus_DestroyTankCrew"), TEXT("ЗНИЩИТИ ЕКІПАЖ"), false);
		AddFlag(TEXT("Bonus_DestroyAPC"), TEXT("ЗНИЩИТИ ЕВАКУАЦІЙНУ МАШИНУ"), false);
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
	EvaluateMissionCompletion();

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
	EvaluateMissionCompletion();

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
	bDroneUseEvaluationPending = false;

	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	if (UsedDroneCount < AvailableDroneCount)
	{
		return;
	}

	ShowMissionEventOnScreen(TEXT("Використано всі доступні дрони"), FColor::Yellow);

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

void AMissionScenarioController::EvaluateMissionCompletion()
{
	if (MissionState != EMissionScenarioState::Active)
	{
		return;
	}

	if (MissionType == EMissionScenarioType::MortarPosition)
	{
		if (IsMortarMissionFullyCompleted())
		{
			ShowMissionEventOnScreen(TEXT("Усі цілі на позиції знищено"), FColor::Green);
			FinishMission(true);
		}

		return;
	}

	if (MissionType == EMissionScenarioType::SPGStrike)
	{
		if (bSPGEscaped)
		{
			FinishMission(false);
			return;
		}

		if (IsSPGMissionFullyCompleted())
		{
			ShowMissionEventOnScreen(TEXT("Умови завершення місії виконано"), FColor::Green);
			FinishMission(true);
		}

		return;
	}

	if (MissionType == EMissionScenarioType::TankAttack)
	{
		if (bFriendlyPositionDestroyed)
		{
			FinishMission(false);
			return;
		}

		if (IsTankMissionFullyCompleted())
		{
			ShowMissionEventOnScreen(TEXT("Умови завершення місії виконано"), FColor::Green);
			FinishMission(true);
		}

		return;
	}
}
bool AMissionScenarioController::IsMortarMissionFullyCompleted() const
{
	return bMortarDestroyed && bAmmoCrateDestroyed && IsAllEnemyInfantryDestroyed();
}

bool AMissionScenarioController::IsSPGMissionFullyCompleted() const
{
	if (!bSPGNeutralized)
	{
		return false;
	}

	if (bSPGDestroyed && IsCrewFullyDestroyedForMission())
	{
		return true;
	}

	return IsCrewFullyResolvedForMission() && IsEvacVehicleResolved();
}

bool AMissionScenarioController::IsTankMissionFullyCompleted() const
{
	if (!bTankNeutralized)
	{
		return false;
	}

	if (bTankDestroyed && IsCrewFullyDestroyedForMission())
	{
		return true;
	}

	return IsCrewFullyResolvedForMission() && IsEvacVehicleResolved();
}

FName AMissionScenarioController::GetCrewGroupIdForMission() const
{
	if (MissionType == EMissionScenarioType::SPGStrike)
	{
		return SPGCrewGroupId;
	}

	if (MissionType == EMissionScenarioType::TankAttack)
	{
		return TankCrewGroupId;
	}

	if (MissionType == EMissionScenarioType::MortarPosition)
	{
		return EnemyInfantryGroupId;
	}

	return NAME_None;
}

FName AMissionScenarioController::GetCrewBonusFlagIdForMission() const
{
	if (MissionType == EMissionScenarioType::SPGStrike)
	{
		return TEXT("Bonus_DestroySPGCrew");
	}

	if (MissionType == EMissionScenarioType::TankAttack)
	{
		return TEXT("Bonus_DestroyTankCrew");
	}

	if (MissionType == EMissionScenarioType::MortarPosition)
	{
		return TEXT("Bonus_DestroyInfantry");
	}

	return NAME_None;
}

int32 AMissionScenarioController::GetExpectedCrewCountForMission() const
{
	if (MissionType == EMissionScenarioType::SPGStrike)
	{
		return MaxSPGCrewCount;
	}

	if (MissionType == EMissionScenarioType::TankAttack)
	{
		return MaxTankCrewCount;
	}

	if (MissionType == EMissionScenarioType::MortarPosition)
	{
		return MaxEnemyInfantryCount;
	}

	return 0;
}

int32 AMissionScenarioController::GetDestroyedCrewCountForMission() const
{
	const FName CrewGroupId = GetCrewGroupIdForMission();

	if (CrewGroupId.IsNone())
	{
		return 0;
	}

	return CountedUnitsByGroup.FindRef(CrewGroupId);
}

int32 AMissionScenarioController::GetResolvedCrewCountForMission() const
{
	const int32 ExpectedCrew = GetExpectedCrewCountForMission();

	if (ExpectedCrew <= 0)
	{
		return 0;
	}

	return FMath::Clamp(GetDestroyedCrewCountForMission() + EscapedCrewCount, 0, ExpectedCrew);
}

bool AMissionScenarioController::IsCrewFullyDestroyedForMission() const
{
	const int32 ExpectedCrew = GetExpectedCrewCountForMission();

	if (ExpectedCrew <= 0)
	{
		return false;
	}

	return GetDestroyedCrewCountForMission() >= ExpectedCrew;
}

bool AMissionScenarioController::IsCrewFullyResolvedForMission() const
{
	const int32 ExpectedCrew = GetExpectedCrewCountForMission();

	if (ExpectedCrew <= 0)
	{
		return false;
	}

	return GetResolvedCrewCountForMission() >= ExpectedCrew;
}

bool AMissionScenarioController::IsEvacVehicleResolved() const
{
	return bAPCDestroyed || bAPCEscaped;
}

bool AMissionScenarioController::IsAllEnemyInfantryDestroyed() const
{
	return CountedUnitsByGroup.FindRef(EnemyInfantryGroupId) >= MaxEnemyInfantryCount;
}