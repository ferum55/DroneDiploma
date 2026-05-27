#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SignalBoundaryVolume.generated.h"

class UBoxComponent;

UCLASS()
class DIPLOMA_API ASignalBoundaryVolume : public AActor
{
	GENERATED_BODY()

public:
	ASignalBoundaryVolume();

	UBoxComponent* GetBoundaryBox() const { return BoundaryBox; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Signal Boundary", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* BoundaryBox = nullptr;
};