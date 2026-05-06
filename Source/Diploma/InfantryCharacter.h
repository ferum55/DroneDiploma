#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InfantryCharacter.generated.h"

UCLASS()
class DIPLOMA_API AInfantryCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AInfantryCharacter();

    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable)
    bool IsDead() const;

    UFUNCTION(BlueprintCallable)
    void SetRunning(bool bRunning);

    UFUNCTION(BlueprintCallable)
    void FireAtActor(AActor* TargetActor);

    UFUNCTION(BlueprintCallable)
    void FireAtLocation(FVector TargetLocation);

    UFUNCTION(BlueprintCallable)
    AActor* GetInitialMoveTarget() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Infantry")
    UStaticMeshComponent* WeaponMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Health")
    float MaxHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Infantry|Health")
    float Health = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Infantry|Health")
    bool bDead = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Movement")
    float WalkSpeed = 220.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Movement")
    float RunSpeed = 450.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Weapon")
    TSubclassOf<AActor> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Weapon")
    FName WeaponSocketName = TEXT("WeaponSocket");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Weapon")
    FName MuzzleSocketName = TEXT("Muzzle");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Weapon")
    float FireCooldown = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Weapon")
    float AccuracySpreadDegrees = 3.0f;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Infantry|Orders")
    AActor* InitialMoveTarget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Ragdoll")
    float RagdollImpulseRadius = 650.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Infantry|Ragdoll")
    float RagdollImpulseStrength = 850.0f;

private:
    float LastFireTime = -1000.0f;

    void Die(FVector ExplosionOrigin, bool bApplyExplosionImpulse);

    FVector GetMuzzleLocation() const;

    FRotator GetAimRotation(FVector TargetLocation) const;

};