#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPVDronePawn.h"
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
	void ApplyAxisValues(const TArray<float>& AxisValues);

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

	UPROPERTY(meta = (BindWidget))
	UProgressBar* BarPitch;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* BarRoll;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* BarYaw;


	//test
public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextThrottleRaw;
	void ApplyRawThrottle(float RawThrottle);
};