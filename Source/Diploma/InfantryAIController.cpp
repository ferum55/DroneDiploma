#include "InfantryAIController.h"
#include "InfantryCharacter.h"
#include "FPVDronePawn.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationData.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogInfantryAI, Log, All);

AInfantryAIController::AInfantryAIController()
{
}

void AInfantryAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] OnPossess | Controller=%s Pawn=%s"),
        *GetNameSafe(this),
        *GetNameSafe(InPawn));

    NavInitAttempts = 0;

    GetWorldTimerManager().SetTimer(
        NavInitTimerHandle,
        this,
        &AInfantryAIController::TryInitializeAI,
        NavInitRetryDelay,
        false
    );
}

void AInfantryAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] OnMoveCompleted | RequestID=%u Code=%d Flags=%d"),
        RequestID.GetID(),
        static_cast<int32>(Result.Code),
        static_cast<int32>(Result.Flags));
}

void AInfantryAIController::TryInitializeAI()
{
    NavInitAttempts++;

    APawn* ControlledPawn = GetPawn();

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] TryInitializeAI %d/%d | Pawn=%s"),
        NavInitAttempts,
        MaxNavInitAttempts,
        *GetNameSafe(ControlledPawn));

    if (!ControlledPawn)
    {
        return;
    }

    if (!BehaviorTreeAsset)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] BehaviorTreeAsset is null"));
        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] World is null"));
        return;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);

    if (!NavSys)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] NavigationSystem is null"));

        if (NavInitAttempts < MaxNavInitAttempts)
        {
            GetWorldTimerManager().SetTimer(
                NavInitTimerHandle,
                this,
                &AInfantryAIController::TryInitializeAI,
                NavInitRetryDelay,
                false
            );
        }

        return;
    }

    ANavigationData* DefaultNavData = NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] Delayed nav check | DefaultNavData=%s"),
        *GetNameSafe(DefaultNavData));

    if (!DefaultNavData)
    {
        if (NavInitAttempts < MaxNavInitAttempts)
        {
            GetWorldTimerManager().SetTimer(
                NavInitTimerHandle,
                this,
                &AInfantryAIController::TryInitializeAI,
                NavInitRetryDelay,
                false
            );
        }

        return;
    }

    if (!BehaviorTreeAsset->BlackboardAsset)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] BehaviorTree has no BlackboardAsset"));
        return;
    }

    UBlackboardComponent* BlackboardComp = nullptr;
    const bool bBlackboardOk = UseBlackboard(BehaviorTreeAsset->BlackboardAsset, BlackboardComp);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] UseBlackboard=%d BlackboardComp=%s BlackboardAsset=%s"),
        bBlackboardOk ? 1 : 0,
        *GetNameSafe(BlackboardComp),
        *GetNameSafe(BehaviorTreeAsset->BlackboardAsset));

    const bool bTreeStarted = RunBehaviorTree(BehaviorTreeAsset);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] RunBehaviorTree=%d"),
        bTreeStarted ? 1 : 0);

    AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(ControlledPawn);

    if (!Infantry)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] Pawn is not AInfantryCharacter"));
        return;
    }

    HomeLocation = ControlledPawn->GetActorLocation();

    AActor* InitialTarget = Infantry->GetInitialMoveTarget();

    const FVector RawReturnLocation = InitialTarget
        ? InitialTarget->GetActorLocation()
        : HomeLocation;

    bHasReturnPostLocation = TryProjectPointToNavMesh(RawReturnLocation, ReturnPostLocation);

    if (!bHasReturnPostLocation)
    {
        ReturnPostLocation = HomeLocation;
        bHasReturnPostLocation = true;
    }

    FVector ProjectedHomeLocation;

    if (TryProjectPointToNavMesh(HomeLocation, ProjectedHomeLocation))
    {
        SetTargetLocation(ProjectedHomeLocation);
    }

    SetShouldRun(false);
    SetInCover(false);
    SetFireRelocating(false);

    FireBurstsBeforeRelocate = FMath::RandRange(FireRelocateAfterMinBursts, FireRelocateAfterMaxBursts);
    FireBurstsAtCurrentPosition = 0;

    StartThreatUpdates();

    ACharacter* CharacterPawn = Cast<ACharacter>(ControlledPawn);

    if (CharacterPawn && CharacterPawn->GetCharacterMovement())
    {
        UCharacterMovementComponent* MoveComp = CharacterPawn->GetCharacterMovement();

        UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] Movement check | MovementMode=%d MaxWalkSpeed=%.2f UpdatedComponent=%s Velocity=%s"),
            static_cast<int32>(MoveComp->MovementMode),
            MoveComp->MaxWalkSpeed,
            *GetNameSafe(MoveComp->UpdatedComponent),
            *CharacterPawn->GetVelocity().ToString());
    }

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] AI initialization success"));
}

void AInfantryAIController::StartThreatUpdates()
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(ThreatUpdateTimerHandle);

    World->GetTimerManager().SetTimer(
        ThreatUpdateTimerHandle,
        this,
        &AInfantryAIController::UpdateThreatState,
        ThreatUpdateInterval,
        true
    );

    UpdateThreatState();

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] Threat updates started | Interval=%.2f DroneTag=%s"),
        ThreatUpdateInterval,
        *DroneActorTag.ToString());
}

void AInfantryAIController::UpdateThreatState()
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn)
    {
        ClearThreatState();
        return;
    }

    AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(ControlledPawn);

    if (!Infantry || Infantry->IsDead())
    {
        ClearThreatState();
        return;
    }

    AActor* ThreatActor = FindBestThreatActor();

    UBlackboardComponent* BB = GetBlackboardComponent();

    if (!BB)
    {
        return;
    }

    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    if (!ThreatActor)
    {
        if (ShouldReturnToPost())
        {
            BB->SetValueAsBool(ShouldFireKey, false);
            BB->SetValueAsName(AIStateKey, TEXT("ReturnToPost"));
            ApplyMovementForState(TEXT("ReturnToPost"), nullptr);
            UpdateCombatFacing(TEXT("ReturnToPost"), nullptr);
            return;
        }

        if (IsWaitAfterSearchActive())
        {
            BB->SetValueAsBool(ShouldFireKey, false);
            BB->SetValueAsName(AIStateKey, TEXT("WaitAfterSearch"));
            ApplyMovementForState(TEXT("WaitAfterSearch"), nullptr);
            UpdateCombatFacing(TEXT("WaitAfterSearch"), nullptr);
            return;
        }

        ClearThreatState();
        return;
    }

    AFPVDronePawn* ThreatDrone = Cast<AFPVDronePawn>(ThreatActor);

    if (ThreatDrone && (ThreatDrone->IsCrashed() || ThreatDrone->IsKillCamActive()))
    {
        ClearThreatState();
        return;
    }

    bool bVisible = false;
    bool bHeard = false;
    float DistanceCm = 0.0f;

    const bool bDetected = IsThreatDetected(ThreatActor, bVisible, bHeard, DistanceCm);

    UpdateVisualSearchState(bVisible, bHeard, DistanceCm);

    if (!bDetected)
    {
        if (ShouldReturnToPost())
        {
            SetThreatActor(nullptr);

            BB->SetValueAsBool(ShouldFireKey, false);
            BB->SetValueAsBool(ShouldRunKey, false);
            BB->SetValueAsVector(LastKnownThreatLocationKey, ThreatActor->GetActorLocation());
            BB->SetValueAsName(AIStateKey, TEXT("ReturnToPost"));

            ApplyMovementForState(TEXT("ReturnToPost"), ThreatActor);
            UpdateCombatFacing(TEXT("ReturnToPost"), ThreatActor);
            return;
        }

        if (IsWaitAfterSearchActive())
        {
            SetThreatActor(ThreatActor);

            BB->SetValueAsBool(ShouldFireKey, false);
            BB->SetValueAsVector(LastKnownThreatLocationKey, ThreatActor->GetActorLocation());
            BB->SetValueAsName(AIStateKey, TEXT("WaitAfterSearch"));

            ApplyMovementForState(TEXT("WaitAfterSearch"), ThreatActor);
            UpdateCombatFacing(TEXT("WaitAfterSearch"), ThreatActor);
            return;
        }

        if (CanStartVisualSearch(DistanceCm))
        {
            StartVisualSearch(DistanceCm);

            SetThreatActor(ThreatActor);

            BB->SetValueAsBool(ShouldFireKey, false);
            BB->SetValueAsVector(LastKnownThreatLocationKey, ThreatActor->GetActorLocation());
            BB->SetValueAsName(AIStateKey, TEXT("SearchVisual"));

            ApplyMovementForState(TEXT("SearchVisual"), ThreatActor);
            UpdateCombatFacing(TEXT("SearchVisual"), ThreatActor);
            return;
        }

        ClearThreatState();
        return;
    }

    const float DroneSpeedKmh = GetActorSpeedKmh(ThreatActor);
    const float ClosingSpeedKmh = GetThreatClosingSpeedKmh(ThreatActor);
    const float DroneFacingDot = GetThreatFacingDotToPawn(ThreatActor);
    const float ElevationAngleDeg = GetElevationAngleToThreat(ThreatActor);

    FName AIState = ChooseDesiredStateWeighted(
        bVisible,
        bHeard,
        DistanceCm,
        DroneSpeedKmh,
        ClosingSpeedKmh,
        DroneFacingDot,
        ElevationAngleDeg
    );

    if (IsWaitAfterSearchActive() && !bVisible && DistanceCm > CloseThreatDistanceCm)
    {
        AIState = TEXT("WaitAfterSearch");
    }

    const bool bEmergencyClose = DistanceCm <= EmergencyEvadeDistanceCm;
    const bool bImmediateInterrupt = bEmergencyClose && AIState != TEXT("CrouchFire") && AIState != TEXT("PrepareFire");

    AIState = ApplyFireStateTiming(AIState, bImmediateInterrupt);

    const bool bFinalShouldFire =
        AIState == TEXT("CrouchFire") ||
        AIState == TEXT("EvadeSideFire");

    LastThreatSeenTime = CurrentTime;

    if (bVisible)
    {
        bVisualSearchActive = false;
        bWaitAfterSearchActive = false;
    }

    SetThreatActor(ThreatActor);

    BB->SetValueAsBool(ShouldFireKey, bFinalShouldFire);
    BB->SetValueAsVector(LastKnownThreatLocationKey, ThreatActor->GetActorLocation());
    BB->SetValueAsName(AIStateKey, AIState);

    ApplyMovementForState(AIState, ThreatActor);
    UpdateCombatFire(AIState, ThreatActor);
    UpdateCombatFacing(AIState, ThreatActor);

    if (bDebugThreatDetection && CurrentTime - LastThreatDebugLogTime >= ThreatDebugLogInterval)
    {
        LastThreatDebugLogTime = CurrentTime;

        UE_LOG(LogInfantryAI, Warning, TEXT("[INF THREAT] Pawn=%s Threat=%s Detected=1 Visible=%d Heard=%d Dist=%.0fcm Speed=%.1fkmh Closing=%.1fkmh FacingDot=%.2f Elev=%.1f Fire=%d State=%s"),
            *GetNameSafe(ControlledPawn),
            *GetNameSafe(ThreatActor),
            bVisible ? 1 : 0,
            bHeard ? 1 : 0,
            DistanceCm,
            DroneSpeedKmh,
            ClosingSpeedKmh,
            DroneFacingDot,
            ElevationAngleDeg,
            bFinalShouldFire ? 1 : 0,
            *AIState.ToString());
    }
}

void AInfantryAIController::ClearThreatState()
{
    UBlackboardComponent* BB = GetBlackboardComponent();

    if (!BB)
    {
        return;
    }

    SetThreatActor(nullptr);

    BB->SetValueAsBool(ShouldFireKey, false);
    BB->SetValueAsBool(ShouldRunKey, false);
    BB->SetValueAsBool(InCoverKey, false);
    BB->SetValueAsBool(FireRelocatingKey, false);
    BB->SetValueAsName(AIStateKey, TEXT("NoThreat"));

    CurrentAIState = TEXT("NoThreat");
    LockedAIState = TEXT("None");
    LockedAIStateUntilTime = -10000.0f;

    LastChosenDecisionState = TEXT("None");
    DecisionLockedUntilTime = -10000.0f;

    bHasActiveCombatMove = false;
    LastIssuedMoveTarget = FVector::ZeroVector;

    bVisualSearchActive = false;
    VisualSearchUntilTime = -10000.0f;
    LastVisualSearchStartTime = -10000.0f;
    VisualSearchDirectionSign = 1.0f;
    bWaitAfterSearchActive = false;

    ResetCombatFire();
    ResetFireRelocation();
    ResetFireStateTiming();

    AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(GetPawn());

    if (Infantry)
    {
        Infantry->SetAIAnimState(TEXT("NoThreat"));
    }
}

AActor* AInfantryAIController::FindBestThreatActor() const
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return nullptr;
    }

    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn)
    {
        return nullptr;
    }

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(World, DroneActorTag, FoundActors);

    AActor* BestActor = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();

    const FVector PawnLocation = ControlledPawn->GetActorLocation();

    for (AActor* Candidate : FoundActors)
    {
        if (!IsValid(Candidate))
        {
            continue;
        }

        AFPVDronePawn* DronePawn = Cast<AFPVDronePawn>(Candidate);

        if (DronePawn && (DronePawn->IsCrashed() || DronePawn->IsKillCamActive()))
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared(PawnLocation, Candidate->GetActorLocation());

        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            BestActor = Candidate;
        }
    }

    return BestActor;
}

bool AInfantryAIController::IsThreatDetected(AActor* ThreatActor, bool& bOutVisible, bool& bOutHeard, float& OutDistanceCm) const
{
    bOutVisible = false;
    bOutHeard = false;
    OutDistanceCm = 0.0f;

    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn || !ThreatActor)
    {
        return false;
    }

    OutDistanceCm = FVector::Dist(ControlledPawn->GetActorLocation(), ThreatActor->GetActorLocation());

    if (OutDistanceCm <= AudioDetectionRangeCm)
    {
        bOutHeard = true;
    }

    if (OutDistanceCm <= VisualDetectionRangeCm &&
        IsThreatInsideSightCone(ThreatActor) &&
        HasLineOfSightToThreat(ThreatActor))
    {
        bOutVisible = true;
    }

    return bOutVisible || bOutHeard;
}

bool AInfantryAIController::IsThreatInsideSightCone(AActor* ThreatActor) const
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn || !ThreatActor)
    {
        return false;
    }

    FVector EyeLocation;
    FRotator EyeRotation;
    ControlledPawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);

    const FVector ToThreat = ThreatActor->GetActorLocation() - EyeLocation;

    if (ToThreat.SizeSquared() < 1.0f)
    {
        return true;
    }

    const FVector ToThreatDir = ToThreat.GetSafeNormal();

    FVector Forward = ControlledPawn->GetActorForwardVector();
    Forward.Z = 0.0f;

    FVector ToThreat2D = ToThreatDir;
    ToThreat2D.Z = 0.0f;

    if (Forward.SizeSquared() < 0.01f || ToThreat2D.SizeSquared() < 0.01f)
    {
        return false;
    }

    Forward.Normalize();
    ToThreat2D.Normalize();

    const float HorizontalDot = FVector::DotProduct(Forward, ToThreat2D);
    const float HorizontalAngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(HorizontalDot, -1.0f, 1.0f)));

    if (HorizontalAngleDeg > SightHorizontalFOVDeg * 0.5f)
    {
        return false;
    }

    const float HorizontalDistance = FVector(ToThreat.X, ToThreat.Y, 0.0f).Size();
    const float VerticalAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(ToThreat.Z, HorizontalDistance));

    if (FMath::Abs(VerticalAngleDeg) > SightVerticalFOVDeg * 0.5f)
    {
        return false;
    }

    return true;
}

bool AInfantryAIController::HasLineOfSightToThreat(AActor* ThreatActor) const
{
    UWorld* World = GetWorld();
    APawn* ControlledPawn = GetPawn();

    if (!World || !ControlledPawn || !ThreatActor)
    {
        return false;
    }

    FVector EyeLocation;
    FRotator EyeRotation;
    ControlledPawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);

    const FVector TargetLocation = ThreatActor->GetActorLocation();

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(ControlledPawn);
    Params.AddIgnoredActor(ThreatActor);
    Params.AddIgnoredActor(this);
    Params.bTraceComplex = true;

    const bool bHit = World->LineTraceSingleByChannel(
        Hit,
        EyeLocation,
        TargetLocation,
        ECC_Visibility,
        Params
    );

    return !bHit;
}

float AInfantryAIController::GetActorSpeedKmh(const AActor* Actor) const
{
    if (!Actor)
    {
        return 0.0f;
    }

    return Actor->GetVelocity().Size() * 0.036f;
}

float AInfantryAIController::GetThreatClosingSpeedKmh(const AActor* ThreatActor) const
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn || !ThreatActor)
    {
        return 0.0f;
    }

    FVector ThreatVelocity = ThreatActor->GetVelocity();
    ThreatVelocity.Z = 0.0f;

    FVector DirectionFromThreatToPawn = ControlledPawn->GetActorLocation() - ThreatActor->GetActorLocation();
    DirectionFromThreatToPawn.Z = 0.0f;

    if (ThreatVelocity.SizeSquared() < 1.0f || DirectionFromThreatToPawn.SizeSquared() < 1.0f)
    {
        return 0.0f;
    }

    const float ClosingSpeedCms = FVector::DotProduct(
        ThreatVelocity,
        DirectionFromThreatToPawn.GetSafeNormal()
    );

    return FMath::Max(0.0f, ClosingSpeedCms * 0.036f);
}

float AInfantryAIController::GetThreatFacingDotToPawn(const AActor* ThreatActor) const
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn || !ThreatActor)
    {
        return 0.0f;
    }

    FVector ThreatDirection = ThreatActor->GetVelocity();
    ThreatDirection.Z = 0.0f;

    if (ThreatDirection.SizeSquared() < FMath::Square(100.0f))
    {
        ThreatDirection = ThreatActor->GetActorForwardVector();
        ThreatDirection.Z = 0.0f;
    }

    FVector DirectionFromThreatToPawn = ControlledPawn->GetActorLocation() - ThreatActor->GetActorLocation();
    DirectionFromThreatToPawn.Z = 0.0f;

    if (ThreatDirection.SizeSquared() < 1.0f || DirectionFromThreatToPawn.SizeSquared() < 1.0f)
    {
        return 0.0f;
    }

    return FVector::DotProduct(
        ThreatDirection.GetSafeNormal(),
        DirectionFromThreatToPawn.GetSafeNormal()
    );
}

float AInfantryAIController::GetElevationAngleToThreat(const AActor* ThreatActor) const
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn || !ThreatActor)
    {
        return 0.0f;
    }

    const FVector ToThreat = ThreatActor->GetActorLocation() - ControlledPawn->GetActorLocation();
    const FVector ToThreatHorizontal(ToThreat.X, ToThreat.Y, 0.0f);

    return FMath::RadiansToDegrees(FMath::Atan2(ToThreat.Z, ToThreatHorizontal.Size()));
}

FName AInfantryAIController::ChooseDesiredStateWeighted(bool bVisible, bool bHeard, float DistanceCm, float DroneSpeedKmh, float ClosingSpeedKmh, float DroneFacingDot, float ElevationAngleDeg) const
{
    UWorld* World = GetWorld();

    if (World && World->GetTimeSeconds() < DecisionLockedUntilTime && LastChosenDecisionState != TEXT("None"))
    {
        return LastChosenDecisionState;
    }

    const bool bDetected = bVisible || bHeard;
    const bool bClose = DistanceCm <= CloseThreatDistanceCm;
    const bool bFastApproaching = ClosingSpeedKmh >= FastApproachSpeedKmh;
    const bool bFireAngleOk = FMath::Abs(ElevationAngleDeg) <= MaxFireElevationAngleDeg;

    TArray<TPair<FName, float>> Options;

    if (!bDetected)
    {
        return TEXT("NoThreat");
    }

    if (!bClose && bVisible)
    {
        if (bFastApproaching)
        {
            Options.Add(TPair<FName, float>(TEXT("CrouchFire"), 30.0f));
            Options.Add(TPair<FName, float>(TEXT("Disperse"), 70.0f));
        }
        else
        {
            Options.Add(TPair<FName, float>(TEXT("CrouchFire"), bFireAngleOk ? 70.0f : 0.0f));
            Options.Add(TPair<FName, float>(TEXT("Disperse"), 30.0f));
        }
    }
    else if (!bClose && !bVisible && bHeard)
    {
        if (bFastApproaching)
        {
            Options.Add(TPair<FName, float>(TEXT("Disperse"), 90.0f));
            Options.Add(TPair<FName, float>(TEXT("SearchVisual"), 10.0f));
        }
        else
        {
            Options.Add(TPair<FName, float>(TEXT("Disperse"), 70.0f));
            Options.Add(TPair<FName, float>(TEXT("SearchVisual"), CanStartVisualSearch(DistanceCm) ? 30.0f : 0.0f));
        }
    }
    else if (bClose && bVisible)
    {
        if (bFireAngleOk)
        {
            Options.Add(TPair<FName, float>(TEXT("EvadeSide"), bFastApproaching ? 60.0f : 50.0f));
            Options.Add(TPair<FName, float>(TEXT("EvadeSideFire"), bFastApproaching ? 30.0f : 40.0f));
            Options.Add(TPair<FName, float>(TEXT("BackOff"), bFastApproaching ? 10.0f : 20.0f));
        }
        else
        {
            Options.Add(TPair<FName, float>(TEXT("EvadeSide"), 80.0f));
            Options.Add(TPair<FName, float>(TEXT("BackOff"), 20.0f));
        }
    }
    else if (bClose && !bVisible && bHeard)
    {
        Options.Add(TPair<FName, float>(TEXT("EvadeSide"), 70.0f));
        Options.Add(TPair<FName, float>(TEXT("BackOff"), 30.0f));
    }
    else
    {
        Options.Add(TPair<FName, float>(TEXT("Disperse"), 100.0f));
    }

    const FName ChosenState = ChooseWeightedState(Options);

    AInfantryAIController* MutableThis = const_cast<AInfantryAIController*>(this);

    MutableThis->LastChosenDecisionState = ChosenState;

    if (World)
    {
        MutableThis->DecisionLockedUntilTime = World->GetTimeSeconds() + DecisionLockSeconds;
    }

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF DECISION] Pawn=%s Chosen=%s Visible=%d Heard=%d Dist=%.0f Speed=%.1f Closing=%.1f"),
        *GetNameSafe(GetPawn()),
        *ChosenState.ToString(),
        bVisible ? 1 : 0,
        bHeard ? 1 : 0,
        DistanceCm,
        DroneSpeedKmh,
        ClosingSpeedKmh);

    return ChosenState;
}

FName AInfantryAIController::ChooseWeightedState(const TArray<TPair<FName, float>>& Options) const
{
    float TotalWeight = 0.0f;

    for (const TPair<FName, float>& Option : Options)
    {
        TotalWeight += FMath::Max(0.0f, Option.Value);
    }

    if (TotalWeight <= 0.0f)
    {
        return TEXT("Disperse");
    }

    const float Roll = FMath::FRandRange(0.0f, TotalWeight);
    float Accumulated = 0.0f;

    for (const TPair<FName, float>& Option : Options)
    {
        Accumulated += FMath::Max(0.0f, Option.Value);

        if (Roll <= Accumulated)
        {
            return Option.Key;
        }
    }

    return Options.Num() > 0 ? Options.Last().Key : TEXT("Disperse");
}

bool AInfantryAIController::CanStartVisualSearch(float DistanceCm) const
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return false;
    }

    const float CurrentTime = World->GetTimeSeconds();

    const bool bFarEnough = DistanceCm >= VisualSearchAfterDisperseDistanceCm;
    const bool bMoveFinished = !bHasActiveCombatMove || HasReachedLastMoveTarget();
    const bool bCooldownReady = CurrentTime - LastVisualSearchStartTime >= VisualSearchCooldownSeconds;

    return bFarEnough && bMoveFinished && bCooldownReady;
}

void AInfantryAIController::StartVisualSearch(float DistanceCm)
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();

    bVisualSearchActive = true;
    bWaitAfterSearchActive = false;
    LastVisualSearchStartTime = CurrentTime;
    VisualSearchUntilTime = CurrentTime + VisualSearchDurationSeconds;
    VisualSearchDirectionSign = FMath::RandBool() ? 1.0f : -1.0f;

    LastChosenDecisionState = TEXT("SearchVisual");
    DecisionLockedUntilTime = CurrentTime + VisualSearchDurationSeconds;

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF SEARCH] Visual search started | Pawn=%s Distance=%.0f Duration=%.2f"),
        *GetNameSafe(GetPawn()),
        DistanceCm,
        VisualSearchDurationSeconds);
}

void AInfantryAIController::UpdateVisualSearchState(bool bVisible, bool bHeard, float DistanceCm)
{
    UWorld* World = GetWorld();

    if (!World)
    {
        bVisualSearchActive = false;
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();

    if (bVisible)
    {
        bVisualSearchActive = false;
        bWaitAfterSearchActive = false;
        return;
    }

    if (bVisualSearchActive && CurrentTime >= VisualSearchUntilTime)
    {
        bVisualSearchActive = false;
        bWaitAfterSearchActive = true;

        LastChosenDecisionState = TEXT("WaitAfterSearch");
        DecisionLockedUntilTime = CurrentTime + ReturnToPostAfterNoThreatSeconds;

        UE_LOG(LogInfantryAI, Warning, TEXT("[INF SEARCH] Visual search failed, waiting | Pawn=%s"),
            *GetNameSafe(GetPawn()));

        return;
    }

    if (bWaitAfterSearchActive)
    {
        return;
    }

    const bool bHeardOnly = bHeard && !bVisible;

    if (!bVisualSearchActive && bHeardOnly && CanStartVisualSearch(DistanceCm))
    {
        StartVisualSearch(DistanceCm);
    }
}

bool AInfantryAIController::IsVisualSearchActive() const
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return false;
    }

    return bVisualSearchActive && World->GetTimeSeconds() < VisualSearchUntilTime;
}

bool AInfantryAIController::IsWaitAfterSearchActive() const
{
    return bWaitAfterSearchActive;
}

bool AInfantryAIController::ShouldReturnToPost() const
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return false;
    }

    if (!bWaitAfterSearchActive)
    {
        return false;
    }

    return World->GetTimeSeconds() - LastThreatSeenTime >= ReturnToPostAfterNoThreatSeconds;
}

void AInfantryAIController::UpdateVisualSearchFacing()
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn)
    {
        return;
    }

    const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;

    FRotator NewRotation = ControlledPawn->GetActorRotation();
    NewRotation.Yaw += VisualSearchDirectionSign * VisualSearchTurnSpeedDegPerSecond * DeltaSeconds;

    ControlledPawn->SetActorRotation(NewRotation);
}

FName AInfantryAIController::ApplyFireStateTiming(FName DesiredAIState, bool bImmediateInterrupt)
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return DesiredAIState;
    }

    const float CurrentTime = World->GetTimeSeconds();

    if (DesiredAIState == TEXT("CrouchFire"))
    {
        if (!bFireCandidateActive)
        {
            bFireCandidateActive = true;
            FireCandidateStartTime = CurrentTime;

            UE_LOG(LogInfantryAI, Warning, TEXT("[INF FIRE STATE] PrepareFire started | Pawn=%s Delay=%.2f MinExit=%.2f"),
                *GetNameSafe(GetPawn()),
                FireEnterDelaySeconds,
                PrepareFireMinExitSeconds);
        }

        if (!bFireStateCommitted)
        {
            const float PrepareTime = CurrentTime - FireCandidateStartTime;

            if (PrepareTime < FireEnterDelaySeconds)
            {
                return TEXT("PrepareFire");
            }

            bFireStateCommitted = true;
            FireStateEnterTime = CurrentTime;

            UE_LOG(LogInfantryAI, Warning, TEXT("[INF FIRE STATE] CrouchFire committed | Pawn=%s MinExit=%.2f"),
                *GetNameSafe(GetPawn()),
                FireMinExitSeconds);
        }

        return TEXT("CrouchFire");
    }

    if (bFireCandidateActive && !bFireStateCommitted)
    {
        const float PrepareTime = CurrentTime - FireCandidateStartTime;

        if (!bImmediateInterrupt && PrepareTime < PrepareFireMinExitSeconds)
        {
            return TEXT("PrepareFire");
        }

        UE_LOG(LogInfantryAI, Warning, TEXT("[INF FIRE STATE] PrepareFire released | Pawn=%s Time=%.2f Immediate=%d"),
            *GetNameSafe(GetPawn()),
            PrepareTime,
            bImmediateInterrupt ? 1 : 0);

        bFireCandidateActive = false;
        FireCandidateStartTime = -10000.0f;

        return DesiredAIState;
    }

    bFireCandidateActive = false;
    FireCandidateStartTime = -10000.0f;

    if (bFireStateCommitted)
    {
        const float FireStateTime = CurrentTime - FireStateEnterTime;

        if (!bImmediateInterrupt && FireStateTime < FireMinExitSeconds)
        {
            return TEXT("CrouchFire");
        }

        UE_LOG(LogInfantryAI, Warning, TEXT("[INF FIRE STATE] CrouchFire released | Pawn=%s Time=%.2f Immediate=%d"),
            *GetNameSafe(GetPawn()),
            FireStateTime,
            bImmediateInterrupt ? 1 : 0);

        bFireStateCommitted = false;
        FireStateEnterTime = -10000.0f;
    }

    return DesiredAIState;
}

void AInfantryAIController::ResetFireStateTiming()
{
    bFireCandidateActive = false;
    bFireStateCommitted = false;
    FireCandidateStartTime = -10000.0f;
    FireStateEnterTime = -10000.0f;
}

void AInfantryAIController::ApplyMovementForState(FName AIState, AActor* ThreatActor)
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();
    const bool bStateChanged = CurrentAIState != AIState;

    if (!bStateChanged && CurrentTime - LastMovementDecisionTime < MovementDecisionCooldownSeconds)
    {
        return;
    }

    if (!bStateChanged && bHasActiveCombatMove && !HasReachedLastMoveTarget())
    {
        return;
    }

    CurrentAIState = AIState;
    LastMovementDecisionTime = CurrentTime;

    AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(GetPawn());

    if (Infantry)
    {
        Infantry->SetAIAnimState(AIState);
    }

    FVector NewTargetLocation = FVector::ZeroVector;
    bool bHasMoveTarget = false;

    if (AIState == TEXT("SearchVisual"))
    {
        SetShouldRun(false);

        if (bStateChanged)
        {
            StopCombatMovement();
        }

        SetInCover(false);
        return;
    }

    if (AIState == TEXT("WaitAfterSearch"))
    {
        SetShouldRun(false);

        if (bStateChanged)
        {
            StopCombatMovement();
        }

        SetInCover(false);
        return;
    }

    if (AIState == TEXT("PrepareFire") || AIState == TEXT("CrouchFire"))
    {
        SetShouldRun(false);

        if (bStateChanged)
        {
            StopCombatMovement();
        }

        SetInCover(false);
        return;
    }

    if (AIState == TEXT("ReturnToPost"))
    {
        SetShouldRun(false);
        SetInCover(false);

        if (bHasReturnPostLocation)
        {
            NewTargetLocation = ReturnPostLocation;
            bHasMoveTarget = true;
        }
    }
    else if (AIState == TEXT("EvadeSide") || AIState == TEXT("EvadeSideFire"))
    {
        bHasMoveTarget = TryChooseSideEvadeLocation(ThreatActor, NewTargetLocation);

        if (!bHasMoveTarget)
        {
            bHasMoveTarget = TryChooseBackOffLocation(ThreatActor, NewTargetLocation);
        }

        SetShouldRun(true);
        SetInCover(false);
    }
    else if (AIState == TEXT("BackOff"))
    {
        bHasMoveTarget = TryChooseBackOffLocation(ThreatActor, NewTargetLocation);

        if (!bHasMoveTarget)
        {
            bHasMoveTarget = TryChooseDisperseLocation(ThreatActor, NewTargetLocation);
        }

        SetShouldRun(true);
        SetInCover(false);
    }
    else if (AIState == TEXT("Disperse"))
    {
        bHasMoveTarget = TryChooseDisperseLocation(ThreatActor, NewTargetLocation);
        SetShouldRun(true);
        SetInCover(false);
    }
    else
    {
        SetShouldRun(false);
        SetInCover(false);
        bHasActiveCombatMove = false;
        return;
    }

    if (!bHasMoveTarget)
    {
        UE_LOG(LogInfantryAI, Warning, TEXT("[INF MOVE] No target found | Pawn=%s State=%s"),
            *GetNameSafe(GetPawn()),
            *AIState.ToString());
        return;
    }

    if (!IsNewMoveTargetUseful(NewTargetLocation))
    {
        UE_LOG(LogInfantryAI, Warning, TEXT("[INF MOVE] Target skipped as not useful | Pawn=%s State=%s Target=%s"),
            *GetNameSafe(GetPawn()),
            *AIState.ToString(),
            *NewTargetLocation.ToString());
        return;
    }

    LastIssuedMoveTarget = NewTargetLocation;
    bHasActiveCombatMove = true;

    SetTargetLocation(NewTargetLocation);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF MOVE] Pawn=%s State=%s Target=%s"),
        *GetNameSafe(GetPawn()),
        *AIState.ToString(),
        *NewTargetLocation.ToString());
}

void AInfantryAIController::StopCombatMovement()
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn)
    {
        return;
    }

    StopMovement();

    FVector ProjectedLocation;

    if (TryProjectPointToNavMesh(ControlledPawn->GetActorLocation(), ProjectedLocation))
    {
        SetTargetLocation(ProjectedLocation);
        LastIssuedMoveTarget = ProjectedLocation;
    }

    bHasActiveCombatMove = false;
}

bool AInfantryAIController::TryChooseDisperseLocation(AActor* ThreatActor, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    APawn* ControlledPawn = GetPawn();

    if (!World || !ControlledPawn)
    {
        return false;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);

    if (!NavSys)
    {
        return false;
    }

    const FVector PawnLocation = ControlledPawn->GetActorLocation();

    FVector BaseDir = FVector::ZeroVector;

    if (ThreatActor)
    {
        BaseDir = PawnLocation - ThreatActor->GetActorLocation();
    }
    else
    {
        BaseDir = PawnLocation - HomeLocation;
    }

    BaseDir.Z = 0.0f;

    if (BaseDir.SizeSquared() < 1.0f)
    {
        BaseDir = ControlledPawn->GetActorForwardVector();
        BaseDir.Z = 0.0f;
    }

    BaseDir.Normalize();

    for (int32 i = 0; i < MovementCandidateCount; i++)
    {
        const float AngleDeg = FMath::RandRange(-80.0f, 80.0f);
        const float Distance = FMath::RandRange(DisperseMinDistanceCm, DisperseMaxDistanceCm);
        const FVector CandidateDir = BaseDir.RotateAngleAxis(AngleDeg, FVector::UpVector).GetSafeNormal();
        const FVector RawCandidate = PawnLocation + CandidateDir * Distance;

        FVector Projected;

        if (!ProjectCandidateToNav(RawCandidate, Projected))
        {
            continue;
        }

        if (!IsLocationFarEnoughFromAllies(Projected))
        {
            continue;
        }

        OutLocation = Projected;
        return true;
    }

    return false;
}

bool AInfantryAIController::TryChooseSideEvadeLocation(AActor* ThreatActor, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    APawn* ControlledPawn = GetPawn();

    if (!World || !ControlledPawn || !ThreatActor)
    {
        return false;
    }

    const FVector PawnLocation = ControlledPawn->GetActorLocation();

    FVector ThreatDirection = ThreatActor->GetVelocity();
    ThreatDirection.Z = 0.0f;

    if (ThreatDirection.SizeSquared() < 1.0f)
    {
        ThreatDirection = ThreatActor->GetActorForwardVector();
        ThreatDirection.Z = 0.0f;
    }

    if (ThreatDirection.SizeSquared() < 1.0f)
    {
        ThreatDirection = PawnLocation - ThreatActor->GetActorLocation();
        ThreatDirection.Z = 0.0f;
    }

    if (ThreatDirection.SizeSquared() < 1.0f)
    {
        return false;
    }

    ThreatDirection.Normalize();

    const FVector RightDir = FVector::CrossProduct(FVector::UpVector, ThreatDirection).GetSafeNormal();
    const float SideSign = FMath::RandBool() ? 1.0f : -1.0f;
    const FVector BaseDir = RightDir * SideSign;

    for (int32 i = 0; i < MovementCandidateCount; i++)
    {
        const float AngleDeg = FMath::RandRange(-35.0f, 35.0f);
        const float Distance = FMath::RandRange(EvadeSideDistanceCm * 0.75f, EvadeSideDistanceCm * 1.25f);
        const FVector CandidateDir = BaseDir.RotateAngleAxis(AngleDeg, FVector::UpVector).GetSafeNormal();
        const FVector RawCandidate = PawnLocation + CandidateDir * Distance;

        FVector Projected;

        if (!ProjectCandidateToNav(RawCandidate, Projected))
        {
            continue;
        }

        if (!IsLocationFarEnoughFromAllies(Projected))
        {
            continue;
        }

        OutLocation = Projected;
        return true;
    }

    return false;
}

bool AInfantryAIController::TryChooseBackOffLocation(AActor* ThreatActor, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    APawn* ControlledPawn = GetPawn();

    if (!World || !ControlledPawn || !ThreatActor)
    {
        return false;
    }

    const FVector PawnLocation = ControlledPawn->GetActorLocation();

    FVector AwayDir = PawnLocation - ThreatActor->GetActorLocation();
    AwayDir.Z = 0.0f;

    if (AwayDir.SizeSquared() < 1.0f)
    {
        AwayDir = ControlledPawn->GetActorForwardVector();
        AwayDir.Z = 0.0f;
    }

    AwayDir.Normalize();

    for (int32 i = 0; i < MovementCandidateCount; i++)
    {
        const float AngleDeg = FMath::RandRange(-45.0f, 45.0f);
        const float Distance = FMath::RandRange(BackOffMinDistanceCm, BackOffMaxDistanceCm);
        const FVector CandidateDir = AwayDir.RotateAngleAxis(AngleDeg, FVector::UpVector).GetSafeNormal();
        const FVector RawCandidate = PawnLocation + CandidateDir * Distance;

        FVector Projected;

        if (!ProjectCandidateToNav(RawCandidate, Projected))
        {
            continue;
        }

        if (!IsLocationFarEnoughFromAllies(Projected))
        {
            continue;
        }

        OutLocation = Projected;
        return true;
    }

    return false;
}

bool AInfantryAIController::TryChooseFireRelocationLocation(AActor* ThreatActor, FVector& OutLocation) const
{
    return TryChooseSideEvadeLocation(ThreatActor, OutLocation);
}

bool AInfantryAIController::ProjectCandidateToNav(const FVector& RawCandidate, FVector& OutLocation) const
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return false;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);

    if (!NavSys)
    {
        return false;
    }

    FNavLocation NavLocation;

    const bool bProjected = NavSys->ProjectPointToNavigation(
        RawCandidate,
        NavLocation,
        FVector(700.0f, 700.0f, 1200.0f)
    );

    if (!bProjected)
    {
        return false;
    }

    OutLocation = NavLocation.Location;
    return true;
}

bool AInfantryAIController::IsLocationFarEnoughFromAllies(const FVector& Location) const
{
    UWorld* World = GetWorld();
    APawn* ControlledPawn = GetPawn();

    if (!World || !ControlledPawn)
    {
        return true;
    }

    for (TActorIterator<AInfantryCharacter> It(World); It; ++It)
    {
        AInfantryCharacter* OtherInfantry = *It;

        if (!OtherInfantry || OtherInfantry == ControlledPawn || OtherInfantry->IsDead())
        {
            continue;
        }

        const float Distance = FVector::Dist(Location, OtherInfantry->GetActorLocation());

        if (Distance < MinAllySpacingCm)
        {
            return false;
        }
    }

    return true;
}

bool AInfantryAIController::HasReachedLastMoveTarget() const
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn || !bHasActiveCombatMove)
    {
        return true;
    }

    return FVector::Dist2D(ControlledPawn->GetActorLocation(), LastIssuedMoveTarget) <= ReachedCombatMoveDistanceCm;
}

bool AInfantryAIController::IsNewMoveTargetUseful(const FVector& NewTargetLocation) const
{
    if (!bHasActiveCombatMove)
    {
        return true;
    }

    return FVector::Dist2D(NewTargetLocation, LastIssuedMoveTarget) >= MinNewMoveTargetDistanceCm;
}

void AInfantryAIController::SetShouldRun(bool bShouldRun)
{
    UBlackboardComponent* BB = GetBlackboardComponent();

    if (BB)
    {
        BB->SetValueAsBool(ShouldRunKey, bShouldRun);
    }

    AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(GetPawn());

    if (Infantry)
    {
        Infantry->SetRunning(bShouldRun);
    }
}

void AInfantryAIController::SetInCover(bool bInCover)
{
    UBlackboardComponent* BB = GetBlackboardComponent();

    if (BB)
    {
        BB->SetValueAsBool(InCoverKey, bInCover);
    }
}

void AInfantryAIController::UpdateCombatFacing(FName AIState, AActor* ThreatActor)
{
    if (AIState == TEXT("SearchVisual"))
    {
        UpdateVisualSearchFacing();
        return;
    }

    const bool bFaceThreat =
        AIState == TEXT("PrepareFire") ||
        AIState == TEXT("CrouchFire") ||
        AIState == TEXT("EvadeSide") ||
        AIState == TEXT("EvadeSideFire");

    if (bFaceThreat && ThreatActor)
    {
        RotatePawnTowardLocation(ThreatActor->GetActorLocation());
        return;
    }

    const bool bFaceMoveTarget =
        AIState == TEXT("Disperse") ||
        AIState == TEXT("BackOff") ||
        AIState == TEXT("ReturnToPost");

    if (bFaceMoveTarget && bHasActiveCombatMove)
    {
        RotatePawnTowardLocation(LastIssuedMoveTarget);
    }
}

void AInfantryAIController::RotatePawnTowardLocation(const FVector& TargetLocation)
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn)
    {
        return;
    }

    FVector ToTarget = TargetLocation - ControlledPawn->GetActorLocation();
    ToTarget.Z = 0.0f;

    if (ToTarget.SizeSquared() < 100.0f)
    {
        return;
    }

    const FRotator TargetRotation = ToTarget.GetSafeNormal().Rotation();
    const FRotator CurrentRotation = ControlledPawn->GetActorRotation();
    const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.016f;

    const FRotator NewRotation = FMath::RInterpTo(
        CurrentRotation,
        FRotator(0.0f, TargetRotation.Yaw, 0.0f),
        DeltaSeconds,
        CombatFacingInterpSpeed
    );

    ControlledPawn->SetActorRotation(NewRotation);
}

void AInfantryAIController::UpdateCombatFire(FName AIState, AActor* ThreatActor)
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(GetPawn());

    if (!Infantry || Infantry->IsDead() || !ThreatActor)
    {
        ResetCombatFire();
        return;
    }

    if (AIState == TEXT("EvadeSideFire"))
    {
        UpdatePanicFire(ThreatActor);
        return;
    }

    if (AIState != TEXT("CrouchFire"))
    {
        ResetCombatFire();
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();

    if (CurrentTime - LastAIStartBurstTime < AIStartBurstIntervalSeconds)
    {
        return;
    }

    LastAIStartBurstTime = CurrentTime;

    const bool bStarted = Infantry->StartFireBurst(ThreatActor);

    if (bStarted)
    {
        UE_LOG(LogInfantryAI, Warning, TEXT("[INF FIRE] Burst started | Pawn=%s Threat=%s"),
            *GetNameSafe(Infantry),
            *GetNameSafe(ThreatActor));
    }
}

void AInfantryAIController::UpdatePanicFire(AActor* ThreatActor)
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(GetPawn());

    if (!Infantry || Infantry->IsDead() || !ThreatActor)
    {
        return;
    }

    const float CurrentTime = World->GetTimeSeconds();

    if (CurrentTime - LastPanicBurstTime < PanicBurstIntervalSeconds)
    {
        return;
    }

    const float SpreadDegrees = CalculatePanicFireSpreadDegrees(ThreatActor);

    const bool bStarted = Infantry->StartFireBurstWithSpread(ThreatActor, SpreadDegrees);

    if (!bStarted)
    {
        return;
    }

    LastPanicBurstTime = CurrentTime;

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF PANIC BURST] Pawn=%s Threat=%s Spread=%.1f Interval=%.2f"),
        *GetNameSafe(Infantry),
        *GetNameSafe(ThreatActor),
        SpreadDegrees,
        PanicBurstIntervalSeconds);
}

float AInfantryAIController::CalculatePanicFireSpreadDegrees(AActor* ThreatActor) const
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn || !ThreatActor)
    {
        return PanicFireMaxSpreadDegrees;
    }

    const FVector PawnLocation = ControlledPawn->GetActorLocation();
    const FVector ThreatLocation = ThreatActor->GetActorLocation();

    const float DistanceCm = FVector::Dist(PawnLocation, ThreatLocation);
    const float DistanceM = DistanceCm / 100.0f;

    const FVector ThreatVelocity = ThreatActor->GetVelocity();
    const float DroneSpeedKmh = ThreatVelocity.Size() * 0.036f;

    FVector ToThreat = ThreatLocation - PawnLocation;

    if (ToThreat.SizeSquared() < 1.0f)
    {
        return PanicFireMaxSpreadDegrees;
    }

    const FVector ToThreatDir = ToThreat.GetSafeNormal();

    const FVector ThreatVelocityDir = ThreatVelocity.SizeSquared() > 1.0f
        ? ThreatVelocity.GetSafeNormal()
        : FVector::ZeroVector;

    const float RadialDot = FMath::Abs(FVector::DotProduct(ThreatVelocityDir, ToThreatDir));
    const float CrossingFactor = 1.0f - RadialDot;

    const float DistancePenalty = FMath::Clamp((DistanceM - 10.0f) * 0.18f, 0.0f, 6.0f);
    const float SpeedPenalty = FMath::Clamp((DroneSpeedKmh - 20.0f) * 0.12f, 0.0f, 8.0f);
    const float CrossingPenalty = CrossingFactor * 8.0f;
    const float MovementPenalty = 8.0f;

    const float FinalSpread =
        PanicFireBaseSpreadDegrees +
        DistancePenalty +
        SpeedPenalty +
        CrossingPenalty +
        MovementPenalty;

    return FMath::Clamp(FinalSpread, PanicFireBaseSpreadDegrees, PanicFireMaxSpreadDegrees);
}

void AInfantryAIController::ResetCombatFire()
{
    LastAIStartBurstTime = -10000.0f;
    LastPanicBurstTime = -10000.0f;
}

void AInfantryAIController::BeginFireRelocation()
{
    bFireRelocating = false;
    SetFireRelocating(false);
}

void AInfantryAIController::UpdateFireRelocationState()
{
    bFireRelocating = false;
    SetFireRelocating(false);
}

void AInfantryAIController::ResetFireRelocation()
{
    bFireRelocating = false;
    FireRelocationUntilTime = -10000.0f;
    SetFireRelocating(false);
}

void AInfantryAIController::SetFireRelocating(bool bRelocating)
{
    UBlackboardComponent* BB = GetBlackboardComponent();

    if (BB)
    {
        BB->SetValueAsBool(FireRelocatingKey, bRelocating);
    }
}

void AInfantryAIController::SetTargetLocation(const FVector& Location)
{
    UBlackboardComponent* BB = GetBlackboardComponent();

    if (!BB)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] SetTargetLocation failed: BlackboardComponent is null"));
        return;
    }

    BB->SetValueAsVector(TargetLocationKey, Location);

    const FVector StoredLocation = BB->GetValueAsVector(TargetLocationKey);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] SetTargetLocation | Key=%s Location=%s Stored=%s"),
        *TargetLocationKey.ToString(),
        *Location.ToString(),
        *StoredLocation.ToString());
}

void AInfantryAIController::SetThreatActor(AActor* ThreatActor)
{
    UBlackboardComponent* BB = GetBlackboardComponent();

    if (!BB)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] SetThreatActor failed: BlackboardComponent is null"));
        return;
    }

    UObject* PreviousThreat = BB->GetValueAsObject(ThreatActorKey);

    BB->SetValueAsObject(ThreatActorKey, ThreatActor);
    BB->SetValueAsBool(HasThreatKey, ThreatActor != nullptr);

    if (PreviousThreat != ThreatActor)
    {
        UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] SetThreatActor | Threat=%s HasThreat=%d"),
            *GetNameSafe(ThreatActor),
            ThreatActor ? 1 : 0);
    }
}

bool AInfantryAIController::TryProjectPointToNavMesh(const FVector& RawLocation, FVector& OutProjectedLocation) const
{
    UWorld* World = GetWorld();

    if (!World)
    {
        OutProjectedLocation = FVector::ZeroVector;
        return false;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);

    if (!NavSys)
    {
        OutProjectedLocation = FVector::ZeroVector;
        return false;
    }

    ANavigationData* DefaultNavData = NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);

    if (!DefaultNavData)
    {
        OutProjectedLocation = FVector::ZeroVector;
        return false;
    }

    FNavLocation ProjectedLocation;

    const bool bProjected = NavSys->ProjectPointToNavigation(
        RawLocation,
        ProjectedLocation,
        FVector(1000.0f, 1000.0f, 1000.0f),
        DefaultNavData
    );

    OutProjectedLocation = bProjected ? ProjectedLocation.Location : FVector::ZeroVector;

    return bProjected;
}

void AInfantryAIController::LogNavigationToTarget(const FVector& TargetLocation) const
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
        World,
        ControlledPawn->GetActorLocation(),
        TargetLocation,
        ControlledPawn
    );

    if (!Path)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] Path check failed: Path is null"));
        return;
    }

    float PathLength = 0.0f;

    for (int32 i = 1; i < Path->PathPoints.Num(); i++)
    {
        PathLength += FVector::Dist(Path->PathPoints[i - 1], Path->PathPoints[i]);
    }

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] Path check | IsValid=%d IsPartial=%d Points=%d Length=%.2f"),
        Path->IsValid() ? 1 : 0,
        Path->IsPartial() ? 1 : 0,
        Path->PathPoints.Num(),
        PathLength);
}