#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DiplomaPawn.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "FPVHUDWidget.generated.h"


class UTextBlock;
class UCanvasPanel;

UCLASS()
class DIPLOMA_API UFPVHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ApplyTelemetry(const FDroneTelemetry& InTelemetry);

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
	UTextBlock* Text_PitchArrow;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_PitchValue;

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* Canvas_HorizonRoot;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float CameraPitchOffsetDeg = 20.f;

	UPROPERTY(EditAnywhere, Category = "HUD")
	float PixelsPerPitchDegree = 4.f;

	//Compass
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_CompassStrip = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* CompassMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
	float CompassZeroOffset = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
	bool bInvertCompassDirection = false;
};