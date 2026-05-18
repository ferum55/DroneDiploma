#include "GraphicsSettingsLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "RHI.h"

namespace
{
	bool ParseResolutionString(const FString& Option, FIntPoint& OutResolution)
	{
		FString Clean = Option;
		Clean.ReplaceInline(TEXT(" "), TEXT(""));
		Clean.ReplaceInline(TEXT("X"), TEXT("x"));

		int32 BracketIndex = INDEX_NONE;
		if (Clean.FindChar(TEXT('('), BracketIndex))
		{
			Clean = Clean.Left(BracketIndex);
		}

		TArray<FString> Parts;
		Clean.ParseIntoArray(Parts, TEXT("x"), true);

		if (Parts.Num() < 2)
		{
			return false;
		}

		const int32 Width = FCString::Atoi(*Parts[0]);
		const int32 Height = FCString::Atoi(*Parts[1]);

		if (Width <= 0 || Height <= 0)
		{
			return false;
		}

		OutResolution = FIntPoint(Width, Height);
		return true;
	}

	FString ResolutionToString(const FIntPoint& Resolution)
	{
		return FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y);
	}

	FString GetAspectLabel(int32 Width, int32 Height)
	{
		if (Width <= 0 || Height <= 0)
		{
			return TEXT("Unknown");
		}

		const float Ratio = static_cast<float>(Width) / static_cast<float>(Height);

		if (FMath::IsNearlyEqual(Ratio, 16.0f / 9.0f, 0.02f))
		{
			return TEXT("16:9");
		}

		if (FMath::IsNearlyEqual(Ratio, 16.0f / 10.0f, 0.02f))
		{
			return TEXT("16:10");
		}

		if (FMath::IsNearlyEqual(Ratio, 4.0f / 3.0f, 0.02f))
		{
			return TEXT("4:3");
		}

		if (FMath::IsNearlyEqual(Ratio, 21.0f / 9.0f, 0.04f))
		{
			return TEXT("21:9");
		}

		if (FMath::IsNearlyEqual(Ratio, 5.0f / 4.0f, 0.02f))
		{
			return TEXT("5:4");
		}

		int32 A = Width;
		int32 B = Height;

		while (B != 0)
		{
			const int32 R = A % B;
			A = B;
			B = R;
		}

		const int32 Divisor = FMath::Max(A, 1);
		return FString::Printf(TEXT("%d:%d"), Width / Divisor, Height / Divisor);
	}

	EWindowMode::Type DisplayModeStringToEnum(const FString& Option)
	{
		if (Option.Equals(TEXT("Windowed"), ESearchCase::IgnoreCase))
		{
			return EWindowMode::Windowed;
		}

		if (Option.Equals(TEXT("Fullscreen Windowed"), ESearchCase::IgnoreCase) || Option.Equals(TEXT("Borderless"), ESearchCase::IgnoreCase))
		{
			return EWindowMode::WindowedFullscreen;
		}

		return EWindowMode::Fullscreen;
	}

	FString DisplayModeEnumToString(EWindowMode::Type Mode)
	{
		if (Mode == EWindowMode::Windowed)
		{
			return TEXT("Windowed");
		}

		if (Mode == EWindowMode::WindowedFullscreen)
		{
			return TEXT("Fullscreen Windowed");
		}

		return TEXT("Fullscreen");
	}

	int32 QualityStringToLevel(const FString& Option)
	{
		if (Option.Equals(TEXT("Low"), ESearchCase::IgnoreCase))
		{
			return 0;
		}

		if (Option.Equals(TEXT("Medium"), ESearchCase::IgnoreCase))
		{
			return 1;
		}

		if (Option.Equals(TEXT("Epic"), ESearchCase::IgnoreCase) || Option.Equals(TEXT("Ultra"), ESearchCase::IgnoreCase))
		{
			return 3;
		}

		return 2;
	}

	FString QualityLevelToString(int32 Level)
	{
		if (Level <= 0)
		{
			return TEXT("Low");
		}

		if (Level == 1)
		{
			return TEXT("Medium");
		}

		return TEXT("High");
	}

	UGameUserSettings* GetSettings()
	{
		return GEngine ? GEngine->GetGameUserSettings() : nullptr;
	}
}

TArray<FString> UGraphicsSettingsLibrary::GetAvailableScreenResolutions()
{
	TArray<FIntPoint> UniqueResolutions;
	FScreenResolutionArray RHIResolutions;

	if (RHIGetAvailableResolutions(RHIResolutions, false))
	{
		for (const FScreenResolutionRHI& Resolution : RHIResolutions)
		{
			if (Resolution.Width < 1024 || Resolution.Height < 720)
			{
				continue;
			}

			const FIntPoint Point(Resolution.Width, Resolution.Height);

			if (!UniqueResolutions.Contains(Point))
			{
				UniqueResolutions.Add(Point);
			}
		}
	}

	if (UniqueResolutions.Num() == 0)
	{
		UniqueResolutions.Add(FIntPoint(1920, 1080));
		UniqueResolutions.Add(FIntPoint(1600, 900));
		UniqueResolutions.Add(FIntPoint(1280, 720));
	}

	UniqueResolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
		{
			if (A.X == B.X)
			{
				return A.Y > B.Y;
			}

			return A.X > B.X;
		});

	TArray<FString> Result;
	for (const FIntPoint& Resolution : UniqueResolutions)
	{
		Result.Add(ResolutionToString(Resolution));
	}

	return Result;
}

TArray<FString> UGraphicsSettingsLibrary::GetAvailableScreenResolutionsByAspect(const FString& AspectOption)
{
	TArray<FString> AllResolutions = GetAvailableScreenResolutions();

	if (AspectOption.IsEmpty())
	{
		return AllResolutions;
	}

	TArray<FString> Result;

	for (const FString& Resolution : AllResolutions)
	{
		if (GetAspectFromResolutionString(Resolution).Equals(AspectOption, ESearchCase::IgnoreCase))
		{
			Result.Add(Resolution);
		}
	}

	if (Result.Num() == 0)
	{
		return AllResolutions;
	}

	return Result;
}

FString UGraphicsSettingsLibrary::GetAspectFromResolutionString(const FString& ResolutionOption)
{
	FIntPoint Resolution;
	if (!ParseResolutionString(ResolutionOption, Resolution))
	{
		return TEXT("Unknown");
	}

	return GetAspectLabel(Resolution.X, Resolution.Y);
}

FString UGraphicsSettingsLibrary::GetCurrentResolutionOption()
{
	UGameUserSettings* Settings = GetSettings();

	TArray<FString> AvailableResolutions = GetAvailableScreenResolutions();

	if (!Settings)
	{
		return AvailableResolutions.Num() > 0 ? AvailableResolutions[0] : TEXT("1920x1080");
	}

	const FIntPoint Resolution = Settings->GetScreenResolution();

	if (Resolution.X > 0 && Resolution.Y > 0)
	{
		const FString CurrentResolution = ResolutionToString(Resolution);

		if (AvailableResolutions.Contains(CurrentResolution))
		{
			return CurrentResolution;
		}
	}

	if (AvailableResolutions.Num() > 0)
	{
		return AvailableResolutions[0];
	}

	return TEXT("1920x1080");
}

FString UGraphicsSettingsLibrary::GetCurrentDisplayModeOption()
{
	UGameUserSettings* Settings = GetSettings();

	if (!Settings)
	{
		return TEXT("Fullscreen");
	}

	return DisplayModeEnumToString(Settings->GetFullscreenMode());
}

FString UGraphicsSettingsLibrary::GetCurrentQualityOption()
{
	UGameUserSettings* Settings = GetSettings();

	if (!Settings)
	{
		return TEXT("High");
	}

	return QualityLevelToString(Settings->GetOverallScalabilityLevel());
}

void UGraphicsSettingsLibrary::ApplyGraphicsSettings(const FString& ResolutionOption, const FString& DisplayModeOption, const FString& QualityOption, bool bSaveSettings)
{
	UGameUserSettings* Settings = GetSettings();

	if (!Settings)
	{
		return;
	}

	FIntPoint Resolution;
	if (ParseResolutionString(ResolutionOption, Resolution))
	{
		Settings->SetScreenResolution(Resolution);
	}

	Settings->SetFullscreenMode(DisplayModeStringToEnum(DisplayModeOption));
	Settings->SetOverallScalabilityLevel(QualityStringToLevel(QualityOption));
	Settings->ApplySettings(false);

	if (bSaveSettings)
	{
		Settings->SaveSettings();
	}
}

void UGraphicsSettingsLibrary::ResetGraphicsSettings()
{
	UGameUserSettings* Settings = GetSettings();

	if (!Settings)
	{
		return;
	}

	TArray<FString> Resolutions = GetAvailableScreenResolutions();
	FIntPoint Resolution(1920, 1080);

	if (Resolutions.Num() > 0)
	{
		ParseResolutionString(Resolutions[0], Resolution);
	}

	Settings->SetScreenResolution(Resolution);
	Settings->SetFullscreenMode(EWindowMode::Fullscreen);
	Settings->SetOverallScalabilityLevel(2);
	Settings->ApplySettings(false);
	Settings->SaveSettings();
}