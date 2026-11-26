#include "UI/MRDrivingHUD.h"

#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Game/MRGameMode.h"
#include "Vehicles/MRNitrousComponent.h"
#include "Vehicles/MRVehiclePawn.h"

void AMRDrivingHUD::DrawHUD()
{
	Super::DrawHUD();

	const AMRVehiclePawn* Car = Cast<AMRVehiclePawn>(GetOwningPawn());
	if (!Car || !Canvas)
	{
		return;
	}

	const float Width = Canvas->ClipX;
	const float Height = Canvas->ClipY;
	const float UIScale = FMath::Max(Height / 1080.f, 0.5f);

	if (bShowControlsHint)
	{
		DrawText(TEXT("WASD / Arrows: drive   SPACE: nitro   SHIFT: handbrake   R: reset   ENTER: restart run"),
			FLinearColor::White, 24.f * UIScale, 20.f * UIScale, nullptr, 1.2f * UIScale);
	}

	// Speed + gear, bottom right.
	const int32 Speed = FMath::RoundToInt(FMath::Abs(Car->GetSpeedKmh()));
	const int32 Gear = Car->GetCurrentGear();
	const FString GearText = Gear < 0 ? TEXT("R") : (Gear == 0 ? TEXT("N") : FString::FromInt(Gear));

	const float Right = Width - 320.f * UIScale;
	const float Bottom = Height - 170.f * UIScale;
	DrawText(FString::Printf(TEXT("%d"), Speed), NeonCyan, Right, Bottom, nullptr, 5.f * UIScale);
	DrawText(TEXT("KM/H"), FLinearColor::White, Right + 190.f * UIScale, Bottom + 40.f * UIScale, nullptr, 1.5f * UIScale);
	DrawText(GearText, NeonPink, Right + 190.f * UIScale, Bottom, nullptr, 2.5f * UIScale);

	// Nitrous bar.
	if (const UMRNitrousComponent* Nitrous = Car->GetNitrous())
	{
		const float BarX = Right;
		const float BarY = Bottom + 90.f * UIScale;
		const float BarW = 280.f * UIScale;
		const float BarH = 16.f * UIScale;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), BarX - 2.f, BarY - 2.f, BarW + 4.f, BarH + 4.f);
		DrawRect(Nitrous->IsBoosting() ? NeonPink : NeonCyan, BarX, BarY, BarW * Nitrous->GetNitrousFraction(), BarH);
		DrawText(TEXT("NITRO"), FLinearColor::White, BarX, BarY + BarH + 4.f * UIScale, nullptr, 1.1f * UIScale);
	}

	DrawRun(Width, Height, UIScale);
}

void AMRDrivingHUD::DrawRun(float Width, float Height, float UIScale)
{
	const AMRGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AMRGameMode>() : nullptr;
	if (!GameMode || GameMode->GetNumCheckpoints() == 0)
	{
		return; // free roam
	}

	const EMRRunState State = GameMode->GetRunState();
	const int32 Total = GameMode->GetNumCheckpoints();
	const float Center = Width * 0.5f;

	// Time left and the next gate, top centre. The clock turns pink in the last 10 seconds.
	const float TimeLeft = GameMode->GetTimeLeft();
	const FLinearColor ClockColour = (State == EMRRunState::Running && TimeLeft <= 10.f) ? NeonPink : NeonCyan;
	DrawText(FString::Printf(TEXT("%.1f"), TimeLeft), ClockColour, Center - 70.f * UIScale, 30.f * UIScale, nullptr, 4.f * UIScale);
	if (State == EMRRunState::Running)
	{
		const int32 Next = FMath::Min(GameMode->GetNextCheckpointOrder() + 1, Total);
		const FString Label = Next == Total ? TEXT("FINISH") : FString::Printf(TEXT("CHECKPOINT %d / %d"), Next, Total);
		DrawText(Label, FLinearColor::White, Center - 110.f * UIScale, 110.f * UIScale, nullptr, 1.5f * UIScale);
	}

	// Result banner.
	if (State == EMRRunState::Won || State == EMRRunState::Lost)
	{
		const bool bWon = State == EMRRunState::Won;
		const float BannerY = Height * 0.38f;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.65f), 0.f, BannerY - 20.f * UIScale, Width, 190.f * UIScale);
		const FString Title = bWon ? FString::Printf(TEXT("RUN COMPLETE  %.2f s"), GameMode->GetElapsed()) : FString(TEXT("TIME UP"));
		DrawText(Title, bWon ? NeonCyan : NeonPink, Center - 260.f * UIScale, BannerY, nullptr, 3.5f * UIScale);
		DrawText(TEXT("ENTER / START: run again"), FLinearColor::White, Center - 150.f * UIScale, BannerY + 110.f * UIScale, nullptr, 1.4f * UIScale);
	}
}
