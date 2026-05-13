#include "ManualRadialDamage.h"
#include "InfantryCharacter.h"
#include "MissionObjectiveActor.h"

#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"

void FManualRadialDamage::Apply(
	UWorld* World,
	const FVector& ExplosionOrigin,
	AActor* DamageCauser,
	AController* InstigatorController,
	float MaxDamage,
	float MinDamage,
	float InnerRadiusCm,
	float OuterRadiusCm,
	float Falloff
)
{
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* TargetActor = *It;

		if (!ShouldReceiveDamage(TargetActor))
		{
			continue;
		}

		if (TargetActor == DamageCauser)
		{
			continue;
		}

		const float DistanceCm = GetDistanceToActor(TargetActor, ExplosionOrigin);

		if (DistanceCm > OuterRadiusCm)
		{
			continue;
		}

		const float Damage = CalculateDamage(DistanceCm, MaxDamage, MinDamage, InnerRadiusCm, OuterRadiusCm, Falloff);

		if (Damage <= 0.0f)
		{
			continue;
		}

		FRadialDamageEvent DamageEvent;
		DamageEvent.Origin = ExplosionOrigin;
		DamageEvent.Params = FRadialDamageParams(MaxDamage, MinDamage, InnerRadiusCm, OuterRadiusCm, Falloff);

		TargetActor->TakeDamage(Damage, DamageEvent, InstigatorController, DamageCauser);

		UE_LOG(LogTemp, Warning,
			TEXT("[MANUAL RADIAL DAMAGE] Target=%s Class=%s Distance=%.1f Damage=%.1f Origin=%s"),
			*GetNameSafe(TargetActor),
			*GetNameSafe(TargetActor->GetClass()),
			DistanceCm,
			Damage,
			*ExplosionOrigin.ToString());
	}
}

bool FManualRadialDamage::ShouldReceiveDamage(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return false;
	}

	if (AInfantryCharacter* Infantry = Cast<AInfantryCharacter>(TargetActor))
	{
		return !Infantry->IsDead();
	}

	if (AMissionObjectiveActor* Objective = Cast<AMissionObjectiveActor>(TargetActor))
	{
		return !Objective->IsDestroyed();
	}

	return false;
}

float FManualRadialDamage::GetDistanceToActor(AActor* TargetActor, const FVector& ExplosionOrigin)
{
	if (!TargetActor)
	{
		return TNumericLimits<float>::Max();
	}

	float BestDistance = FVector::Dist(ExplosionOrigin, TargetActor->GetActorLocation());

	TArray<UPrimitiveComponent*> Components;
	TargetActor->GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
		{
			continue;
		}

		FVector ClosestPoint = Component->GetComponentLocation();
		const float ClosestDistance = Component->GetClosestPointOnCollision(ExplosionOrigin, ClosestPoint);

		if (ClosestDistance >= 0.0f)
		{
			BestDistance = FMath::Min(BestDistance, FVector::Dist(ExplosionOrigin, ClosestPoint));
		}
	}

	return BestDistance;
}

float FManualRadialDamage::CalculateDamage(float DistanceCm, float MaxDamage, float MinDamage, float InnerRadiusCm, float OuterRadiusCm, float Falloff)
{
	if (DistanceCm <= InnerRadiusCm)
	{
		return MaxDamage;
	}

	if (DistanceCm >= OuterRadiusCm)
	{
		return MinDamage;
	}

	const float Alpha = FMath::Clamp(
		(DistanceCm - InnerRadiusCm) / FMath::Max(1.0f, OuterRadiusCm - InnerRadiusCm),
		0.0f,
		1.0f
	);

	const float FalloffAlpha = FMath::Pow(Alpha, FMath::Max(0.01f, Falloff));

	return FMath::Lerp(MaxDamage, MinDamage, FalloffAlpha);
}