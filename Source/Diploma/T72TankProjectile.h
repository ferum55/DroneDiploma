#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T72TankProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UParticleSystem;

UCLASS()
class DIPLOMA_API AT72TankProjectile : public AActor
{
	GENERATED_BODY()

public:
	AT72TankProjectile();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Damage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float DamageRadiusCm = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TSubclassOf<UDamageType> DamageTypeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	UParticleSystem* ExplosionEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	float LifeSeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
	float ExplosionEffectScale = 2.5f;

protected:
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	bool bExploded = false;

	void Explode(const FVector& Location);
};