#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DiplomaHUD.generated.h"

class UDroneTelemetryWidget;
class UUserWidget;

UCLASS()
class DIPLOMA_API ADiplomaHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

protected:
	UPROPERTY(EditAnywhere, Category = "HUD")
	float ReticleSize = 120.f;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float ReticleGap = 35.f;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float LineThickness = 2.f;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float DotSize = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UDroneTelemetryWidget> TelemetryWidgetClass;

	UPROPERTY()
	UDroneTelemetryWidget* TelemetryWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> FPVHUDWidgetClass;

	UPROPERTY()
	UUserWidget* FPVHUDWidget = nullptr;

	float ThrottleMin = 999.f;
	float ThrottleMax = -999.f;
};