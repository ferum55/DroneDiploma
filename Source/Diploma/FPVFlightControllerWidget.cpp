#include "FPVFlightControllerWidget.h"
#include "FPVControllerSettingsSaveGame.h"
#include "FPVDronePawn.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/InputKeySelector.h"
#include "Components/Slider.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UFPVFlightControllerWidget::NativeConstruct()
{
    Super::NativeConstruct();

    FindTargetDronePawn();
    LoadSavedSettings();
    BuildNumericControls();


    if (InputKeySelector_Arm)
    {
        ConfigureKeySelector(InputKeySelector_Arm);
        InputKeySelector_Arm->OnKeySelected.AddDynamic(this, &UFPVFlightControllerWidget::OnArmKeySelected);
    }

    if (InputKeySelector_BombArm)
    {
        ConfigureKeySelector(InputKeySelector_BombArm);
        InputKeySelector_BombArm->OnKeySelected.AddDynamic(this, &UFPVFlightControllerWidget::OnBombArmKeySelected);
    }

    if (InputKeySelector_Flightmode)
    {
        ConfigureKeySelector(InputKeySelector_Flightmode);
        InputKeySelector_Flightmode->OnKeySelected.AddDynamic(this, &UFPVFlightControllerWidget::OnFlightModeKeySelected);
    }

    for (FNumericControl& Control : NumericControls)
    {
        if (Control.Text)
        {
            Control.Text->OnTextCommitted.AddDynamic(this, &UFPVFlightControllerWidget::OnAnyTextCommitted);
        }
    }

    RefreshAllControls();
    PushWorkingSettingsToDrone(true);
}

void UFPVFlightControllerWidget::ApplyControllerSettingsConfirmed()
{
    ReadAllTextControls();
    WorkingSettings.Clamp();
    PushWorkingSettingsToDrone(true);
    SaveWorkingSettings();
    RefreshAllControls();
}

void UFPVFlightControllerWidget::ResetControllerSettingsConfirmed()
{
    WorkingSettings = FFPVControllerSettings::MakeDefault();
    RefreshAllControls();
    PushWorkingSettingsToDrone(true);
    SaveWorkingSettings();
}

void UFPVFlightControllerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bUpdatingControls)
    {
        return;
    }

    bool bChanged = false;

    for (FNumericControl& Control : NumericControls)
    {
        if (!Control.Slider || !Control.Setter)
        {
            continue;
        }

        const float SliderValue = FMath::Clamp(Control.Slider->GetValue(), Control.MinValue, Control.MaxValue);

        if (!FMath::IsNearlyEqual(SliderValue, Control.LastSliderValue, 0.0001f))
        {
            Control.LastSliderValue = SliderValue;
            Control.Setter(SliderValue);
            SetNumericText(Control, SliderValue);
            bChanged = true;
        }
    }

    if (bChanged)
    {
        WorkingSettings.Clamp();
        PushWorkingSettingsToDrone(false);
    }
}

void UFPVFlightControllerWidget::FindTargetDronePawn()
{
    TargetDronePawn = Cast<AFPVDronePawn>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void UFPVFlightControllerWidget::LoadSavedSettings()
{
    WorkingSettings = FFPVControllerSettings::MakeDefault();

    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        if (UFPVControllerSettingsSaveGame* Loaded = Cast<UFPVControllerSettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0)))
        {
            WorkingSettings = Loaded->Settings;
        }
    }

    WorkingSettings.Clamp();
}

void UFPVFlightControllerWidget::SaveWorkingSettings()
{
    WorkingSettings.Clamp();

    UFPVControllerSettingsSaveGame* Save = Cast<UFPVControllerSettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(UFPVControllerSettingsSaveGame::StaticClass()));
    if (!Save)
    {
        return;
    }

    Save->Settings = WorkingSettings;
    UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, 0);
}

void UFPVFlightControllerWidget::BuildNumericControls()
{
    NumericControls.Reset();

    AddNumericControl(Slider_PitchP, EditableText_PitchP, 0.0f, 1.0f, 3, [this]() { return WorkingSettings.Pitch.P; }, [this](float V) { WorkingSettings.Pitch.P = V; });
    AddNumericControl(Slider_PitchI, EditableText_PitchI, 0.0f, 1.0f, 3, [this]() { return WorkingSettings.Pitch.I; }, [this](float V) { WorkingSettings.Pitch.I = V; });
    AddNumericControl(Slider_PitchD, EditableText_PitchD, 0.0f, 1.0f, 3, [this]() { return WorkingSettings.Pitch.D; }, [this](float V) { WorkingSettings.Pitch.D = V; });
    AddNumericControl(Slider_PitchDeadZone, EditableText_PitchDeadZone, 0.0f, 0.5f, 3, [this]() { return WorkingSettings.Pitch.DeadZone; }, [this](float V) { WorkingSettings.Pitch.DeadZone = V; });
    AddNumericControl(Slider_PitchExpo, EditableText_PitchExpo, 1.0f, 3.0f, 2, [this]() { return WorkingSettings.Pitch.Expo; }, [this](float V) { WorkingSettings.Pitch.Expo = V; });
    AddNumericControl(Slider_PitchMaxRate, EditableText_PitchMaxRate, 1.0f, 1000.0f, 0, [this]() { return WorkingSettings.Pitch.MaxRateDegPerSec; }, [this](float V) { WorkingSettings.Pitch.MaxRateDegPerSec = V; });

    AddNumericControl(Slider_RollP, EditableText_RollP, 0.0f, 1.0f, 3, [this]() { return WorkingSettings.Roll.P; }, [this](float V) { WorkingSettings.Roll.P = V; });
    AddNumericControl(Slider_RollI, EditableText_RollI, 0.0f, 1.0f, 3, [this]() { return WorkingSettings.Roll.I; }, [this](float V) { WorkingSettings.Roll.I = V; });
    AddNumericControl(Slider_RollD, EditableText_RollD, 0.0f, 1.0f, 3, [this]() { return WorkingSettings.Roll.D; }, [this](float V) { WorkingSettings.Roll.D = V; });
    AddNumericControl(Slider_RollDeadZone, EditableText_RollDeadZone, 0.0f, 0.5f, 3, [this]() { return WorkingSettings.Roll.DeadZone; }, [this](float V) { WorkingSettings.Roll.DeadZone = V; });
    AddNumericControl(Slider_RollExpo, EditableText_RollExpo, 1.0f, 3.0f, 2, [this]() { return WorkingSettings.Roll.Expo; }, [this](float V) { WorkingSettings.Roll.Expo = V; });
    AddNumericControl(Slider_RollMaxRate, EditableText_RollMaxRate, 1.0f, 1000.0f, 0, [this]() { return WorkingSettings.Roll.MaxRateDegPerSec; }, [this](float V) { WorkingSettings.Roll.MaxRateDegPerSec = V; });

    AddNumericControl(Slider_YawP, EditableText_YawP, 0.0f, 1.0f, 3, [this]() { return WorkingSettings.Yaw.P; }, [this](float V) { WorkingSettings.Yaw.P = V; });
    AddNumericControl(Slider_YawI, EditableText_YawI, 0.0f, 1.0f, 3, [this]() { return WorkingSettings.Yaw.I; }, [this](float V) { WorkingSettings.Yaw.I = V; });
    AddNumericControl(Slider_YawD, EditableText_YawD, 0.0f, 1.0f, 3, [this]() { return WorkingSettings.Yaw.D; }, [this](float V) { WorkingSettings.Yaw.D = V; });
    AddNumericControl(Slider_YawDeadZone, EditableText_YawDeadZone, 0.0f, 0.5f, 3, [this]() { return WorkingSettings.Yaw.DeadZone; }, [this](float V) { WorkingSettings.Yaw.DeadZone = V; });
    AddNumericControl(Slider_YawExpo, EditableText_YawExpo, 1.0f, 3.0f, 2, [this]() { return WorkingSettings.Yaw.Expo; }, [this](float V) { WorkingSettings.Yaw.Expo = V; });
    AddNumericControl(Slider_YawMaxRate, EditableText_YawMaxRate, 1.0f, 1000.0f, 0, [this]() { return WorkingSettings.Yaw.MaxRateDegPerSec; }, [this](float V) { WorkingSettings.Yaw.MaxRateDegPerSec = V; });

    AddNumericControl(Slider_ThrottleDeadZone, EditableText_ThrottleDeadZone, 0.0f, 0.5f, 3, [this]() { return WorkingSettings.ThrottleDeadZone; }, [this](float V) { WorkingSettings.ThrottleDeadZone = V; });
    AddNumericControl(Slider_ThrottleExpo, EditableText_ThrottleExpo, 1.0f, 3.0f, 2, [this]() { return WorkingSettings.ThrottleExpo; }, [this](float V) { WorkingSettings.ThrottleExpo = V; });
}

void UFPVFlightControllerWidget::AddNumericControl(USlider* Slider, UEditableText* Text, float MinValue, float MaxValue, int32 Decimals, TFunction<float()> Getter, TFunction<void(float)> Setter)
{
    FNumericControl Control;
    Control.Slider = Slider;
    Control.Text = Text;
    Control.MinValue = MinValue;
    Control.MaxValue = MaxValue;
    Control.Decimals = Decimals;
    Control.Getter = Getter;
    Control.Setter = Setter;
    NumericControls.Add(Control);
}

void UFPVFlightControllerWidget::RefreshAllControls()
{
    bUpdatingControls = true;

    WorkingSettings.Clamp();

    for (FNumericControl& Control : NumericControls)
    {
        if (!Control.Getter || !Control.Setter)
        {
            continue;
        }

        const float Value = FMath::Clamp(Control.Getter(), Control.MinValue, Control.MaxValue);
        Control.Setter(Value);
        Control.LastSliderValue = Value;

        if (Control.Slider)
        {
            Control.Slider->SetMinValue(Control.MinValue);
            Control.Slider->SetMaxValue(Control.MaxValue);
            Control.Slider->SetValue(Value);
        }

        SetNumericText(Control, Value);
    }

    RefreshKeySelectors();
    bUpdatingControls = false;
}

void UFPVFlightControllerWidget::RefreshKeySelectors()
{
    if (InputKeySelector_Arm)
    {
        InputKeySelector_Arm->SetSelectedKey(FInputChord(WorkingSettings.ArmKey));
    }

    if (InputKeySelector_BombArm)
    {
        InputKeySelector_BombArm->SetSelectedKey(FInputChord(WorkingSettings.BombArmKey));
    }

    if (InputKeySelector_Flightmode)
    {
        InputKeySelector_Flightmode->SetSelectedKey(FInputChord(WorkingSettings.CycleFlightModeKey));
    }
}

void UFPVFlightControllerWidget::ConfigureKeySelector(UInputKeySelector* Selector)
{
    if (!Selector)
    {
        return;
    }

    Selector->SetAllowGamepadKeys(false);
    Selector->SetAllowModifierKeys(false);
}

void UFPVFlightControllerWidget::PushWorkingSettingsToDrone(bool bApplyInputMappings)
{
    WorkingSettings.Clamp();
    FindTargetDronePawn();

    if (TargetDronePawn)
    {
        TargetDronePawn->ApplyControllerSettings(WorkingSettings, bApplyInputMappings);
    }
}

void UFPVFlightControllerWidget::ReadAllTextControls()
{
    bUpdatingControls = true;

    for (FNumericControl& Control : NumericControls)
    {
        if (!Control.Setter)
        {
            continue;
        }

        float Value = 0.0f;
        if (TryReadTextValue(Control.Text, Value))
        {
            Value = FMath::Clamp(Value, Control.MinValue, Control.MaxValue);
            Control.Setter(Value);
            Control.LastSliderValue = Value;

            if (Control.Slider)
            {
                Control.Slider->SetValue(Value);
            }

            SetNumericText(Control, Value);
        }
        else if (Control.Getter)
        {
            SetNumericText(Control, Control.Getter());
        }
    }

    WorkingSettings.Clamp();
    bUpdatingControls = false;
}

void UFPVFlightControllerWidget::SetNumericText(FNumericControl& Control, float Value) const
{
    if (!Control.Text)
    {
        return;
    }

    if (Control.Text->HasKeyboardFocus())
    {
        return;
    }

    Control.Text->SetText(FText::FromString(FormatNumericValue(Control, Value)));
}

FString UFPVFlightControllerWidget::FormatNumericValue(const FNumericControl& Control, float Value) const
{
    if (Control.Decimals <= 0)
    {
        return FString::Printf(TEXT("%.0f"), Value);
    }

    if (Control.Decimals == 2)
    {
        return FString::Printf(TEXT("%.2f"), Value);
    }

    return FString::Printf(TEXT("%.3f"), Value);
}

bool UFPVFlightControllerWidget::TryReadTextValue(UEditableText* Text, float& OutValue) const
{
    if (!Text)
    {
        return false;
    }

    FString Raw = Text->GetText().ToString();
    Raw.TrimStartAndEndInline();
    Raw.ReplaceInline(TEXT(","), TEXT("."));

    return LexTryParseString(OutValue, *Raw);
}

bool UFPVFlightControllerWidget::IsAllowedKeyboardKey(const FKey& Key) const
{
    if (!Key.IsValid())
    {
        return false;
    }

    if (Key == EKeys::AnyKey)
    {
        return false;
    }

    if (Key.IsGamepadKey() || Key.IsMouseButton())
    {
        return false;
    }

    return true;
}

void UFPVFlightControllerWidget::SelectKeyboardKey(UInputKeySelector* Selector, FKey& TargetKey, const FInputChord& SelectedKey)
{
    const FKey Key = SelectedKey.Key;

    if (!IsAllowedKeyboardKey(Key))
    {
        RefreshKeySelectors();
        return;
    }

    TargetKey = Key;

    if (Selector)
    {
        Selector->SetSelectedKey(FInputChord(Key));
    }
}

void UFPVFlightControllerWidget::OnApplyClicked()
{
    ReadAllTextControls();
    PushWorkingSettingsToDrone(true);
    SaveWorkingSettings();
    RefreshAllControls();
}

void UFPVFlightControllerWidget::OnResetClicked()
{
    WorkingSettings = FFPVControllerSettings::MakeDefault();
    RefreshAllControls();
    PushWorkingSettingsToDrone(true);
    SaveWorkingSettings();
}

void UFPVFlightControllerWidget::OnBackClicked()
{
    RemoveFromParent();

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
}

void UFPVFlightControllerWidget::OnAnyTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    ReadAllTextControls();
    PushWorkingSettingsToDrone(false);
}

void UFPVFlightControllerWidget::OnArmKeySelected(FInputChord SelectedKey)
{
    SelectKeyboardKey(InputKeySelector_Arm, WorkingSettings.ArmKey, SelectedKey);
}

void UFPVFlightControllerWidget::OnBombArmKeySelected(FInputChord SelectedKey)
{
    SelectKeyboardKey(InputKeySelector_BombArm, WorkingSettings.BombArmKey, SelectedKey);
}

void UFPVFlightControllerWidget::OnFlightModeKeySelected(FInputChord SelectedKey)
{
    SelectKeyboardKey(InputKeySelector_Flightmode, WorkingSettings.CycleFlightModeKey, SelectedKey);
}
