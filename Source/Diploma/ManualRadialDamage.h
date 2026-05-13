#pragma once

#include "CoreMinimal.h"

class DIPLOMA_API FManualRadialDamage
{
public:
	static void Apply(
		UWorld* World,
		const FVector& ExplosionOrigin,
		AActor* DamageCauser,
		AController* InstigatorController,
		float MaxDamage,
		float MinDamage,
		float InnerRadiusCm,
		float OuterRadiusCm,
		float Falloff
	);

private:
	static bool ShouldReceiveDamage(AActor* TargetActor);
	static float GetDistanceToActor(AActor* TargetActor, const FVector& ExplosionOrigin);
	static float CalculateDamage(float DistanceCm, float MaxDamage, float MinDamage, float InnerRadiusCm, float OuterRadiusCm, float Falloff);
};