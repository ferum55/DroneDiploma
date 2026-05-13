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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile|Damage")
	bool bUseManualExplosionDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile|Damage")
	float ManualExplosionMaxDamage = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile|Damage")
	float ManualExplosionMinDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile|Damage")
	float ManualExplosionInnerRadiusCm = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile|Damage")
	float ManualExplosionOuterRadiusCm = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile|Damage")
	float ManualExplosionFalloff = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "T72 Projectile|Damage")
	float ManualExplosionOriginZOffset = 60.0f;

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