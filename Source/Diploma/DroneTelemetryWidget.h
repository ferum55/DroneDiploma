#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DiplomaPawn.h"
#include "DroneTelemetryWidget.generated.h"

class UTextBlock;
class UProgressBar;

UCLASS()
class DIPLOMA_API UDroneTelemetryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ApplyTelemetry(const FDroneTelemetry& InTelemetry);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextSpeed;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextThrottle;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBaroAlt;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextRadioAlt;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextVerticalSpeed;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* BarThrottle;
};