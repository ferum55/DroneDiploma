#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DiplomaPawn.h"
#include "FPVHUDWidget.generated.h"

class UTextBlock;

UCLASS()
class DIPLOMA_API UFPVHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ApplyTelemetry(const FDroneTelemetry& InTelemetry);

	UFUNCTION(BlueprintCallable)
	void SetFlightModeText(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void SetPrimaryLinkText(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void SetVideoLinkText(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void SetTxPowerText(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void SetArmedText(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void SetBombText(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void SetHeadingText(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void SetCompassText(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void SetTimerText(const FString& InText);

	UFUNCTION(BlueprintCallable)
	void SetPitchCue(const FString& ArrowText, const FString& ValueText);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_PrimaryLink;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_VideoLink;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_TxPower;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Armed;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Bomb;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_CellVoltage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_PackVoltage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_ConsumedMah;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_CurrentAmp;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_FlightMode;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Altitude;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Throttle;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_Timer;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_HeadingDegrees;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_CompassCardinals;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_PitchArrow;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_PitchValue;
};