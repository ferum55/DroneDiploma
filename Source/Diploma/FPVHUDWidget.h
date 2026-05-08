#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DroneTelemetry.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "FPVHUDWidget.generated.h"


class UTextBlock;
class UCanvasPanel;
class URetainerBox;

UCLASS()
class DIPLOMA_API UFPVHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ApplyTelemetry(const FDroneTelemetry& InTelemetry);

protected:

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* Image_PitchArrow = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "FPV|HUD")
	UTexture2D* PitchArrowTexture = nullptr;

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

	UPROPERTY(meta = (BindWidget))
	UProgressBar* BarPackBattery = nullptr;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* BarCellBattery = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_Speed;

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

	UPROPERTY(meta = (BindWidget))
	URetainerBox* Retainer_HUD = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* HUDInterferenceMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
	float CompassZeroOffset = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
	bool bInvertCompassDirection = false;
};