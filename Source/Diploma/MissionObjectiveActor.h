#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissionObjectiveActor.generated.h"

class UStaticMeshComponent;
class UParticleSystem;

UCLASS()
class DIPLOMA_API AMissionObjectiveActor : public AActor
{
	GENERATED_BODY()

public:
	AMissionObjectiveActor();

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "Mission Objective")
	bool IsDestroyed() const;

	UFUNCTION(BlueprintCallable, Category = "Mission Objective")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Mission Objective")
	float GetMaxHealth() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* ObjectiveMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Objective")
	FName ObjectiveId = TEXT("Objective");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Objective")
	bool bPrimaryObjective = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Objective")
	int32 ScoreValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Objective|Health")
	float MaxHealth = 200.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission Objective|Health")
	float Health = 200.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission Objective|Health")
	bool bDestroyed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Objective|FX")
	UParticleSystem* DamageFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Objective|FX")
	UParticleSystem* DestroyedFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Objective|Destroyed")
	bool bHideMeshOnDestroyed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Objective|Destroyed")
	bool bDisableCollisionOnDestroyed = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission Objective")
	void BP_OnObjectiveDamaged(float DamageAmount, float CurrentHealth, float InMaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission Objective")
	void BP_OnObjectiveDestroyed();

private:
	void DestroyObjective();
};