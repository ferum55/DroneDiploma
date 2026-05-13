#include "MissionObjectiveActor.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"

AMissionObjectiveActor::AMissionObjectiveActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ObjectiveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObjectiveMesh"));
	ObjectiveMesh->SetupAttachment(SceneRoot);
	ObjectiveMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ObjectiveMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ObjectiveMesh->SetCollisionResponseToAllChannels(ECR_Block);
}

void AMissionObjectiveActor::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	bDestroyed = false;
}

float AMissionObjectiveActor::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDestroyed || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);

	if (DamageFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DamageFX, GetActorLocation(), GetActorRotation());
	}

	BP_OnObjectiveDamaged(DamageAmount, Health, MaxHealth);

	UE_LOG(LogTemp, Warning, TEXT("[MISSION OBJECTIVE DAMAGE] Actor=%s Id=%s Damage=%.1f Health=%.1f Causer=%s"),
		*GetNameSafe(this),
		*ObjectiveId.ToString(),
		DamageAmount,
		Health,
		*GetNameSafe(DamageCauser));

	if (Health <= 0.0f)
	{
		DestroyObjective();
	}

	return AppliedDamage;
}

bool AMissionObjectiveActor::IsDestroyed() const
{
	return bDestroyed;
}

float AMissionObjectiveActor::GetHealth() const
{
	return Health;
}

float AMissionObjectiveActor::GetMaxHealth() const
{
	return MaxHealth;
}

void AMissionObjectiveActor::DestroyObjective()
{
	if (bDestroyed)
	{
		return;
	}

	bDestroyed = true;
	Health = 0.0f;

	if (DestroyedFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DestroyedFX, GetActorLocation(), GetActorRotation());
	}

	BP_OnObjectiveDestroyed();

	if (bDisableCollisionOnDestroyed)
	{
		SetActorEnableCollision(false);
	}

	if (bHideMeshOnDestroyed && ObjectiveMesh)
	{
		ObjectiveMesh->SetVisibility(false, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("[MISSION OBJECTIVE DESTROYED] Actor=%s Id=%s Primary=%d Score=%d"),
		*GetNameSafe(this),
		*ObjectiveId.ToString(),
		bPrimaryObjective ? 1 : 0,
		ScoreValue);
}