#include "InfantryAIController.h"
#include "InfantryCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/World.h"


#include "EngineUtils.h"
#include "NavigationData.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


#include "TimerManager.h"

#include "Navigation/PathFollowingComponent.h"





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
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] BehaviorTreeAsset is null. Set it in BP_InfantryAIController"));
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
        LogNavigationToTarget(ControlledPawn->GetActorLocation());

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
        else
        {
            UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] NavData still null after %d attempts"), NavInitAttempts);
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

    AActor* InitialTarget = Infantry->GetInitialMoveTarget();

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] InitialMoveTarget=%s"),
        *GetNameSafe(InitialTarget));

    if (!InitialTarget)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] InitialMoveTarget is null"));
        return;
    }

    const FVector RawTargetLocation = InitialTarget->GetActorLocation();
    FVector ProjectedTargetLocation;

    const bool bProjected = TryProjectPointToNavMesh(RawTargetLocation, ProjectedTargetLocation);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] Target projection delayed | Projected=%d Raw=%s Projected=%s"),
        bProjected ? 1 : 0,
        *RawTargetLocation.ToString(),
        *ProjectedTargetLocation.ToString());

    if (!bProjected)
    {
        LogNavigationToTarget(RawTargetLocation);
        return;
    }

    SetTargetLocation(ProjectedTargetLocation);
    LogNavigationToTarget(ProjectedTargetLocation);

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

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] Controller/Pawn check | Controller=%s PawnController=%s Pawn=%s"),
        *GetNameSafe(this),
        *GetNameSafe(ControlledPawn->GetController()),
        *GetNameSafe(ControlledPawn));

    if (bUseDirectMoveTest)
    {
        const EPathFollowingRequestResult::Type MoveResult = MoveToLocation(
            ProjectedTargetLocation,
            100.0f,
            true,
            true,
            true,
            false,
            nullptr,
            true
        );

        UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] Direct MoveToLocation result=%d MoveStatus=%d Destination=%s"),
            static_cast<int32>(MoveResult),
            static_cast<int32>(GetMoveStatus()),
            *ProjectedTargetLocation.ToString());

        return;
    }

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] AI initialization success"));
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
    const FVector QueryExtent(1000.0f, 1000.0f, 1000.0f);

    const bool bProjected = NavSys->ProjectPointToNavigation(
        RawLocation,
        ProjectedLocation,
        QueryExtent,
        DefaultNavData
    );

    OutProjectedLocation = bProjected ? ProjectedLocation.Location : FVector::ZeroVector;

    return bProjected;
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

    BB->SetValueAsObject(ThreatActorKey, ThreatActor);
    BB->SetValueAsBool(HasThreatKey, ThreatActor != nullptr);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] SetThreatActor | Threat=%s HasThreat=%d"),
        *GetNameSafe(ThreatActor),
        ThreatActor ? 1 : 0);
}

void AInfantryAIController::LogNavigationToTarget(const FVector& TargetLocation) const
{
    APawn* ControlledPawn = GetPawn();

    if (!ControlledPawn)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] Nav check failed: ControlledPawn is null"));
        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] Nav check failed: World is null"));
        return;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);

    if (!NavSys)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] Nav check failed: NavigationSystem is null"));
        return;
    }

    ANavigationData* DefaultNavData = NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] NavData | DefaultNavData=%s"),
        *GetNameSafe(DefaultNavData));

    int32 BoundsCount = 0;

    for (TActorIterator<ANavMeshBoundsVolume> It(World); It; ++It)
    {
        ANavMeshBoundsVolume* Volume = *It;
        BoundsCount++;

        const FBox Box = Volume->GetComponentsBoundingBox(true);
        const FVector Size = Box.GetSize();

        UE_LOG(LogInfantryAI, Warning, TEXT("[NAV DIAG] Bounds[%d] Name=%s Loc=%s Scale=%s BoxMin=%s BoxMax=%s Size=%s IsValid=%d"),
            BoundsCount,
            *GetNameSafe(Volume),
            *Volume->GetActorLocation().ToString(),
            *Volume->GetActorScale3D().ToString(),
            *Box.Min.ToString(),
            *Box.Max.ToString(),
            *Size.ToString(),
            Box.IsValid ? 1 : 0);
    }

    UE_LOG(LogInfantryAI, Warning, TEXT("[NAV DIAG] NavMeshBoundsVolume count=%d"), BoundsCount);

    int32 RecastCount = 0;

    for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
    {
        ARecastNavMesh* Recast = *It;
        RecastCount++;

        UE_LOG(LogInfantryAI, Warning, TEXT("[NAV DIAG] Recast[%d] Name=%s Loc=%s TileSizeUU=%.2f CellSize=%.2f CellHeight=%.2f AgentRadius=%.2f AgentHeight=%.2f"),
            RecastCount,
            *GetNameSafe(Recast),
            *Recast->GetActorLocation().ToString(),
            Recast->TileSizeUU,
            Recast->CellSize,
            Recast->CellHeight,
            Recast->AgentRadius,
            Recast->AgentHeight);
    }

    UE_LOG(LogInfantryAI, Warning, TEXT("[NAV DIAG] RecastNavMesh count=%d"), RecastCount);

    if (!DefaultNavData)
    {
        UE_LOG(LogInfantryAI, Error, TEXT("[INF AI] DefaultNavData is null. RecastNavMesh is missing or not main nav data"));
        return;
    }

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] NavData location=%s"),
        *DefaultNavData->GetActorLocation().ToString());

    FNavLocation ProjectedStart;
    FNavLocation ProjectedTarget;

    const FVector QueryExtent(500.0f, 500.0f, 1000.0f);

    const bool bStartOnNav = NavSys->ProjectPointToNavigation(
        ControlledPawn->GetActorLocation(),
        ProjectedStart,
        QueryExtent,
        DefaultNavData
    );

    const bool bTargetOnNav = NavSys->ProjectPointToNavigation(
        TargetLocation,
        ProjectedTarget,
        QueryExtent,
        DefaultNavData
    );

    UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] Nav projection | StartOnNav=%d Start=%s ProjectedStart=%s | TargetOnNav=%d Target=%s ProjectedTarget=%s"),
        bStartOnNav ? 1 : 0,
        *ControlledPawn->GetActorLocation().ToString(),
        *ProjectedStart.Location.ToString(),
        bTargetOnNav ? 1 : 0,
        *TargetLocation.ToString(),
        *ProjectedTarget.Location.ToString());

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

    for (int32 i = 0; i < Path->PathPoints.Num(); i++)
    {
        UE_LOG(LogInfantryAI, Warning, TEXT("[INF AI] Path point %d: %s"),
            i,
            *Path->PathPoints[i].ToString());
    }

     BoundsCount = 0;

    for (TActorIterator<ANavMeshBoundsVolume> It(GetWorld()); It; ++It)
    {
        ANavMeshBoundsVolume* Volume = *It;
        BoundsCount++;

        const FBox Box = Volume->GetComponentsBoundingBox(true);
        const FVector Size = Box.GetSize();

        UE_LOG(LogInfantryAI, Warning, TEXT("[NAV DIAG] Bounds[%d] Name=%s Loc=%s Scale=%s BoxMin=%s BoxMax=%s Size=%s IsValid=%d"),
            BoundsCount,
            *GetNameSafe(Volume),
            *Volume->GetActorLocation().ToString(),
            *Volume->GetActorScale3D().ToString(),
            *Box.Min.ToString(),
            *Box.Max.ToString(),
            *Size.ToString(),
            Box.IsValid ? 1 : 0);
    }

    UE_LOG(LogInfantryAI, Warning, TEXT("[NAV DIAG] NavMeshBoundsVolume count=%d"), BoundsCount);
}

