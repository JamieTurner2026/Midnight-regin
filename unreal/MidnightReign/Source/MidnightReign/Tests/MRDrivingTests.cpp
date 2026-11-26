// Unreal Automation tests. Run in the editor: Tools > Session Frontend >
// Automation, filter "MidnightReign". The engine-independent logic itself is
// covered by core/tests (plain C++); these check the Unreal-side wiring.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Game/MRCheckpoint.h"
#include "Game/MRGameMode.h"
#include "Input/MRInputConfig.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Vehicles/MRNitrousComponent.h"
#include "Vehicles/MRVehiclePawn.h"
#include "Vehicles/MRVehicleTuningData.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMRSettingsConversionTest, "MidnightReign.Vehicles.SettingsConversion",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMRSettingsConversionTest::RunTest(const FString& Parameters)
{
	FMRNitrousSettings NitrousSettings;
	NitrousSettings.Capacity = 150.f;
	NitrousSettings.DrainPerSecond = 30.f;
	NitrousSettings.RechargePerSecond = 5.f;
	NitrousSettings.RechargeDelay = 2.f;
	NitrousSettings.MinToActivate = 20.f;
	const mr::NitrousConfig Core = NitrousSettings.ToCoreConfig();
	TestEqual(TEXT("Capacity"), Core.Capacity, 150.f);
	TestEqual(TEXT("Drain"), Core.DrainPerSec, 30.f);
	TestEqual(TEXT("Recharge"), Core.RechargePerSec, 5.f);
	TestEqual(TEXT("Delay"), Core.RechargeDelay, 2.f);
	TestEqual(TEXT("MinToActivate"), Core.MinToActivate, 20.f);

	FMRSteeringSettings SteeringSettings;
	const mr::SteeringConfig Steering = SteeringSettings.ToCoreConfig();
	TestEqual(TEXT("Full steering at rest"), mr::SteeringScaleForSpeed(0.f, Steering), 1.f);
	TestEqual(TEXT("Reduced steering at speed"), mr::SteeringScaleForSpeed(300.f, Steering), SteeringSettings.MinLockScale);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMRInputConfigTest, "MidnightReign.Input.DefaultBindings",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMRInputConfigTest::RunTest(const FString& Parameters)
{
	UMRInputConfig* Config = NewObject<UMRInputConfig>();
	Config->BuildDefaults();

	TestNotNull(TEXT("Mapping context"), Config->MappingContext.Get());
	TestNotNull(TEXT("Throttle"), Config->ThrottleAction.Get());
	TestNotNull(TEXT("Brake"), Config->BrakeAction.Get());
	TestNotNull(TEXT("Steer"), Config->SteerAction.Get());
	TestNotNull(TEXT("Nitrous"), Config->NitrousAction.Get());
	TestNotNull(TEXT("Handbrake"), Config->HandbrakeAction.Get());

	if (Config->MappingContext)
	{
		bool bSpaceIsNitrous = false;
		bool bWIsThrottle = false;
		for (const FEnhancedActionKeyMapping& Mapping : Config->MappingContext->GetMappings())
		{
			bSpaceIsNitrous |= Mapping.Key == EKeys::SpaceBar && Mapping.Action == Config->NitrousAction;
			bWIsThrottle |= Mapping.Key == EKeys::W && Mapping.Action == Config->ThrottleAction;
		}
		TestTrue(TEXT("Space is nitrous, as in the prototype"), bSpaceIsNitrous);
		TestTrue(TEXT("W is throttle, as in the prototype"), bWIsThrottle);

		bool bEnterRestarts = false;
		for (const FEnhancedActionKeyMapping& Mapping : Config->MappingContext->GetMappings())
		{
			bEnterRestarts |= Mapping.Key == EKeys::Enter && Mapping.Action == Config->RestartAction;
		}
		TestTrue(TEXT("Enter restarts the checkpoint run"), bEnterRestarts);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMRVehicleDefaultsTest, "MidnightReign.Vehicles.PawnDefaults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMRVehicleDefaultsTest::RunTest(const FString& Parameters)
{
	const AMRVehiclePawn* Defaults = GetDefault<AMRVehiclePawn>();
	const UChaosWheeledVehicleMovementComponent* Movement = Defaults->GetWheeledMovement();
	TestNotNull(TEXT("Chaos wheeled movement component"), Movement);
	TestNotNull(TEXT("Nitrous component"), Defaults->GetNitrous());
	if (Movement)
	{
		TestEqual(TEXT("Four wheels"), Movement->WheelSetups.Num(), 4);
		TestTrue(TEXT("Has forward gears"), Movement->TransmissionSetup.ForwardGearRatios.Num() > 0);
		TestTrue(TEXT("Brake reverses when stopped"), Movement->bReverseAsBrake != 0);
	}

	// Tuning asset overrides the movement setup.
	UMRVehicleTuningData* Tuning = NewObject<UMRVehicleTuningData>();
	Tuning->MassKg = 1234.f;
	Tuning->Drivetrain = EMRDrivetrain::AllWheelDrive;
	Tuning->ForwardGearRatios = { 3.f, 2.f, 1.f };
	UChaosWheeledVehicleMovementComponent* Scratch = NewObject<UChaosWheeledVehicleMovementComponent>();
	Tuning->ApplyTo(Scratch);
	TestEqual(TEXT("Mass applied"), Scratch->Mass, 1234.f);
	TestEqual(TEXT("Gears applied"), Scratch->TransmissionSetup.ForwardGearRatios.Num(), 3);
	TestTrue(TEXT("Drivetrain applied"), Scratch->DifferentialSetup.DifferentialType == EVehicleDifferential::AllWheelDrive);
	TestEqual(TEXT("Wheels applied"), Scratch->WheelSetups.Num(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMRCheckpointRunWiringTest, "MidnightReign.Game.CheckpointRunWiring",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMRCheckpointRunWiringTest::RunTest(const FString& Parameters)
{
	// The rules themselves are tested in core/tests; this checks the Unreal side maps them correctly.
	TestTrue(TEXT("Ready"), AMRGameMode::ToRunState(mr::RunState::Ready) == EMRRunState::Ready);
	TestTrue(TEXT("Running"), AMRGameMode::ToRunState(mr::RunState::Running) == EMRRunState::Running);
	TestTrue(TEXT("Won"), AMRGameMode::ToRunState(mr::RunState::Won) == EMRRunState::Won);
	TestTrue(TEXT("Lost"), AMRGameMode::ToRunState(mr::RunState::Lost) == EMRRunState::Lost);

	const AMRGameMode* Defaults = GetDefault<AMRGameMode>();
	TestTrue(TEXT("A positive time limit"), Defaults->TimeLimit > 0.f);
	TestTrue(TEXT("The run starts on play"), Defaults->bStartRunOnPlay);
	TestEqual(TEXT("No gates before play: free roam"), Defaults->GetNumCheckpoints(), 0);

	const AMRCheckpoint* Gate = GetDefault<AMRCheckpoint>();
	TestEqual(TEXT("Default gate order"), Gate->Order, 0);
	TestFalse(TEXT("No gate is next before the run starts"), Gate->IsNext());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
