#include "SignalBoundaryVolume.h"
#include "Components/BoxComponent.h"

ASignalBoundaryVolume::ASignalBoundaryVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	BoundaryBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundaryBox"));
	RootComponent = BoundaryBox;

	BoundaryBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundaryBox->SetHiddenInGame(true);
	BoundaryBox->SetBoxExtent(FVector(50000.f, 50000.f, 30000.f));
}