#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Commands/InputChord.h"
#include "FPVControllerSettingsTypes.h"
#include "FPVFlightControllerWidget.generated.h"

class AFPVDronePawn;
class UButton;
class UEditableText;
class UInputKeySelector;
class USlider;

UCLASS()
class DIPLOMA_API UFPVFlightControllerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "FPV Controller Settings")
    void ApplyControllerSettingsConfirmed();

    UFUNCTION(BlueprintCallable, Category = "FPV Controller Settings")
    void ResetControllerSettingsConfirmed();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UPROPERTY(EditAnywhere, Category = "FPV|Controller")
    FString SaveSlotName = TEXT("FPVControllerSettings");

    UPROPERTY()
    AFPVDronePawn* TargetDronePawn = nullptr;

    FFPVControllerSettings WorkingSettings;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_PitchP = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_PitchP = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_PitchI = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_PitchI = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_PitchD = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_PitchD = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_PitchDeadZone = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_PitchDeadZone = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_PitchExpo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_PitchExpo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_PitchMaxRate = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_PitchMaxRate = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_RollP = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_RollP = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_RollI = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_RollI = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_RollD = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_RollD = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_RollDeadZone = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_RollDeadZone = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_RollExpo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_RollExpo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_RollMaxRate = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_RollMaxRate = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_YawP = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_YawP = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_YawI = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_YawI = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_YawD = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_YawD = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_YawDeadZone = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_YawDeadZone = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_YawExpo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_YawExpo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_YawMaxRate = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_YawMaxRate = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_ThrottleDeadZone = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_ThrottleDeadZone = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USlider* Slider_ThrottleExpo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableText* EditableText_ThrottleExpo = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UInputKeySelector* InputKeySelector_Arm = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UInputKeySelector* InputKeySelector_BombArm = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UInputKeySelector* InputKeySelector_Flightmode = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* Button_Apply = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* Button_Reset = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* Button_Back = nullptr;

    UFUNCTION()
    void OnApplyClicked();

    UFUNCTION()
    void OnResetClicked();

    UFUNCTION()
    void OnBackClicked();

    UFUNCTION()
    void OnAnyTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UFUNCTION()
    void OnArmKeySelected(FInputChord SelectedKey);

    UFUNCTION()
    void OnBombArmKeySelected(FInputChord SelectedKey);

    UFUNCTION()
    void OnFlightModeKeySelected(FInputChord SelectedKey);

    struct FNumericControl
    {
        USlider* Slider = nullptr;
        UEditableText* Text = nullptr;
        float MinValue = 0.0f;
        float MaxValue = 1.0f;
        int32 Decimals = 3;
        float LastSliderValue = 0.0f;
        TFunction<float()> Getter;
        TFunction<void(float)> Setter;
    };

    TArray<FNumericControl> NumericControls;
    bool bUpdatingControls = false;

    void FindTargetDronePawn();
    void LoadSavedSettings();
    void SaveWorkingSettings();
    void BuildNumericControls();
    void AddNumericControl(USlider* Slider, UEditableText* Text, float MinValue, float MaxValue, int32 Decimals, TFunction<float()> Getter, TFunction<void(float)> Setter);
    void RefreshAllControls();
    void RefreshKeySelectors();
    void ConfigureKeySelector(UInputKeySelector* Selector);
    void PushWorkingSettingsToDrone(bool bApplyInputMappings);
    void ReadAllTextControls();
    void SetNumericText(FNumericControl& Control, float Value) const;
    FString FormatNumericValue(const FNumericControl& Control, float Value) const;
    bool TryReadTextValue(UEditableText* Text, float& OutValue) const;
    bool IsAllowedKeyboardKey(const FKey& Key) const;
    void SelectKeyboardKey(UInputKeySelector* Selector, FKey& TargetKey, const FInputChord& SelectedKey);
};
