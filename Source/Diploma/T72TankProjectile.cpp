#include "T72TankProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ManualRadialDamage.h"


AT72TankProjectile::AT72TankProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(12.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetNotifyRigidBodyCollision(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 90500.0f;
	ProjectileMovement->MaxSpeed = 90500.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->SetUpdatedComponent(Collision);
}

void AT72TankProjectile::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentHit.AddDynamic(this, &AT72TankProjectile::OnProjectileHit);
	SetLifeSpan(LifeSeconds);

	UE_LOG(LogTemp, Warning, TEXT("[T72 PROJECTILE] BeginPlay | Name=%s Owner=%s Location=%s Rotation=%s Velocity=%s InitialSpeed=%.1f MaxSpeed=%.1f Gravity=%.2f CollisionEnabled=%d"),
		*GetName(),
		GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"),
		*GetActorLocation().ToString(),
		*GetActorRotation().ToString(),
		*ProjectileMovement->Velocity.ToString(),
		ProjectileMovement->InitialSpeed,
		ProjectileMovement->MaxSpeed,
		ProjectileMovement->ProjectileGravityScale,
		static_cast<int32>(Collision->GetCollisionEnabled()));
}

void AT72TankProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())
	{
		UE_LOG(LogTemp, Warning, TEXT("[T72 PROJECTILE] Ignored hit with owner"));
		return;
	}

	const FVector ExplosionLocation = Hit.ImpactPoint.IsNearlyZero()
		? GetActorLocation()
		: Hit.ImpactPoint;

	AController* DamageInstigator = GetInstigatorController();

	if (!DamageInstigator && GetOwner())
	{
		DamageInstigator = GetOwner()->GetInstigatorController();
	}

	FManualRadialDamage::Apply(
		GetWorld(),
		ExplosionLocation + FVector(0.0f, 0.0f, ManualExplosionOriginZOffset),
		this,
		DamageInstigator,
		ManualExplosionMaxDamage,
		ManualExplosionMinDamage,
		ManualExplosionInnerRadiusCm,
		ManualExplosionOuterRadiusCm,
		ManualExplosionFalloff
	);

	UE_LOG(LogTemp, Warning, TEXT("[T72 PROJECTILE] Hit | Projectile=%s OtherActor=%s OtherComp=%s ImpactPoint=%s DamageOrigin=%s"),
		*GetName(),
		*GetNameSafe(OtherActor),
		*GetNameSafe(OtherComp),
		*ExplosionLocation.ToString(),
		*(ExplosionLocation + FVector(0.0f, 0.0f, ManualExplosionOriginZOffset)).ToString());

	Explode(ExplosionLocation);
}

void AT72TankProjectile::Explode(const FVector& Location)
{
	if (bExploded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[T72 PROJECTILE] Explode skipped, already exploded"));
		return;
	}

	bExploded = true;

	if (ExplosionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ExplosionEffect,
			Location,
			FRotator::ZeroRotator,
			FVector(ExplosionEffectScale),
			true
		);

		UE_LOG(LogTemp, Warning, TEXT("[T72 PROJECTILE] Explosion particle spawned"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[T72 PROJECTILE] ExplosionEffect is NULL. Set it in BP_T72TankProjectile"));
	}

	Destroy();
}