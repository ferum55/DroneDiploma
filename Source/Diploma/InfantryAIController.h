#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "InfantryAIController.generated.h"

class UBehaviorTree;

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

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    UBehaviorTree* BehaviorTreeAsset;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
    FName TargetLocationKey = TEXT("TargetLocation");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
    FName ThreatActorKey = TEXT("ThreatActor");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Blackboard")
    FName HasThreatKey = TEXT("bHasThreat");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Navigation")
    int32 MaxNavInitAttempts = 20;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Navigation")
    float NavInitRetryDelay = 0.25f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Debug")
    bool bUseDirectMoveTest = true;

private:
    FTimerHandle NavInitTimerHandle;
    int32 NavInitAttempts = 0;

    void TryInitializeAI();
    bool TryProjectPointToNavMesh(const FVector& RawLocation, FVector& OutProjectedLocation) const;
    void LogNavigationToTarget(const FVector& TargetLocation) const;
};