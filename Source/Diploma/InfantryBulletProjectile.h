#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InfantryBulletProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class DIPLOMA_API AInfantryBulletProjectile : public AActor
{
    GENERATED_BODY()

private:
    FVector PreviousLocation = FVector::ZeroVector;
public:
    AInfantryBulletProjectile();
    virtual void Tick(float DeltaSeconds) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug")
    bool bDrawDebugTrail = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug")
    float DebugTrailLifeSeconds = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug")
    float DebugTrailThickness = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug")
    float DebugImpactSphereRadius = 8.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
    USphereComponent* CollisionComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bullet")
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    float Damage = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
    float LifeSeconds = 3.0f;

    UFUNCTION()
    void OnBulletHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};