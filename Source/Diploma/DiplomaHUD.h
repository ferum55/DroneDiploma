#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DiplomaHUD.generated.h"

class UFPVHUDWidget;

UCLASS()
class DIPLOMA_API ADiplomaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UFPVHUDWidget> FPVHUDWidgetClass;

	UPROPERTY()
	UFPVHUDWidget* FPVHUDWidget = nullptr;
};