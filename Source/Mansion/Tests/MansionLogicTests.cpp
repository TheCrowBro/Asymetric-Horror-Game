#include "Misc/AutomationTest.h"
#include "Core/MansionTypes.h"
#include "Lighting/IlluminationSubsystem.h"
#include "Survivor/ExposureComponent.h"
#include "Survivor/FearComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#define MansionTestFlags (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
	bool HasReason(uint8 Reasons, ERevealReason Reason)
	{
		return (Reasons & static_cast<uint8>(Reason)) != 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMansionIlluminationFalloffTest, "Mansion.Logic.Illumination.Falloff", MansionTestFlags)
bool FMansionIlluminationFalloffTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Full intensity at the light"), UIlluminationSubsystem::ComputeFalloff(1.f, 1000.f, 0.f), 1.f);
	TestEqual(TEXT("Quarter intensity at half radius"), UIlluminationSubsystem::ComputeFalloff(1.f, 1000.f, 500.f), 0.25f);
	TestEqual(TEXT("Zero at the radius"), UIlluminationSubsystem::ComputeFalloff(1.f, 1000.f, 1000.f), 0.f);
	TestEqual(TEXT("Zero beyond the radius"), UIlluminationSubsystem::ComputeFalloff(1.f, 1000.f, 5000.f), 0.f);
	TestEqual(TEXT("Zero radius is safe"), UIlluminationSubsystem::ComputeFalloff(1.f, 0.f, 0.f), 0.f);
	TestTrue(TEXT("Monotonic"), UIlluminationSubsystem::ComputeFalloff(1.f, 1000.f, 300.f) > UIlluminationSubsystem::ComputeFalloff(1.f, 1000.f, 600.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMansionIlluminationConeTest, "Mansion.Logic.Illumination.Cone", MansionTestFlags)
bool FMansionIlluminationConeTest::RunTest(const FString& Parameters)
{
	const FVector Forward = FVector::ForwardVector;
	TestEqual(TEXT("Dead ahead is full"), UIlluminationSubsystem::ComputeConeFactor(Forward, Forward, 15.f, 30.f), 1.f);
	TestEqual(TEXT("Behind is zero"), UIlluminationSubsystem::ComputeConeFactor(Forward, -Forward, 15.f, 30.f), 0.f);

	const FVector At45 = FRotator(0.f, 45.f, 0.f).Vector();
	TestEqual(TEXT("Outside outer cone is zero"), UIlluminationSubsystem::ComputeConeFactor(Forward, At45, 15.f, 30.f), 0.f);

	const FVector At22 = FRotator(0.f, 22.5f, 0.f).Vector();
	TestEqual(TEXT("Halfway between cones is half"), UIlluminationSubsystem::ComputeConeFactor(Forward, At22, 15.f, 30.f), 0.5f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMansionFearBonusTest, "Mansion.Logic.Fear.RollBonusCap", MansionTestFlags)
bool FMansionFearBonusTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("No fear, no bonus"), FMansionFear::RollBonus(0.f, 100.f, 0.075f), 0.f);
	TestEqual(TEXT("Half fear, half bonus"), FMansionFear::RollBonus(50.f, 100.f, 0.075f), 0.0375f);
	TestEqual(TEXT("Max fear is +7.5%"), FMansionFear::RollBonus(100.f, 100.f, 0.075f), 0.075f);
	TestEqual(TEXT("Over max still capped at +7.5%"), FMansionFear::RollBonus(250.f, 100.f, 0.075f), 0.075f);
	TestEqual(TEXT("Negative fear clamps to zero"), FMansionFear::RollBonus(-10.f, 100.f, 0.075f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMansionFearStepTest, "Mansion.Logic.Fear.Step", MansionTestFlags)
bool FMansionFearStepTest::RunTest(const FString& Parameters)
{
	// Max 100, light >= 0.4, dark < 0.15, light decay 15/s, dark gain 4/s.
	auto Step = [](float Fear, float Light, float Dt)
	{
		return FMansionFear::Step(Fear, Light, Dt, 100.f, 0.4f, 0.15f, 15.f, 4.f);
	};

	TestEqual(TEXT("Darkness raises fear"), Step(10.f, 0.05f, 1.f), 14.f);
	TestEqual(TEXT("Light drains fear quickly"), Step(50.f, 0.8f, 1.f), 35.f);
	TestEqual(TEXT("Dim light leaves fear alone"), Step(50.f, 0.25f, 1.f), 50.f);
	TestEqual(TEXT("Never below zero"), Step(5.f, 1.f, 10.f), 0.f);
	TestEqual(TEXT("Never above max"), Step(99.f, 0.f, 10.f), 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMansionExposureCampingTest, "Mansion.Logic.Exposure.Camping", MansionTestFlags)
bool FMansionExposureCampingTest::RunTest(const FString& Parameters)
{
	FExposureParams Params;
	Params.CampThreshold = 20.f;
	Params.MoveTolerance = 150.f;
	Params.DarkThreshold = 0.15f;

	FExposureTracker Tracker;
	const FVector Spot(0.f, 0.f, 0.f);
	float Now = 0.f;
	uint8 Reasons = 0;

	for (int32 Step = 0; Step < 95; ++Step) // 19 seconds
	{
		Now += 0.2f;
		Reasons = Tracker.Update(Spot + FVector(Step % 2 ? 50.f : -50.f, 0.f, 0.f), 1.f, Now, 0.2f, Params);
	}
	TestFalse(TEXT("Shuffling in place for 19s is not yet camping"), HasReason(Reasons, ERevealReason::Camping));

	for (int32 Step = 0; Step < 10; ++Step)
	{
		Now += 0.2f;
		Reasons = Tracker.Update(Spot, 1.f, Now, 0.2f, Params);
	}
	TestTrue(TEXT("Past 20s in one spot is camping"), HasReason(Reasons, ERevealReason::Camping));

	Now += 0.2f;
	Reasons = Tracker.Update(Spot + FVector(400.f, 0.f, 0.f), 1.f, Now, 0.2f, Params);
	TestFalse(TEXT("Moving away resets camping"), HasReason(Reasons, ERevealReason::Camping));
	TestEqual(TEXT("Stationary timer reset"), Tracker.GetStationaryTime(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMansionExposureDarkTest, "Mansion.Logic.Exposure.Dark", MansionTestFlags)
bool FMansionExposureDarkTest::RunTest(const FString& Parameters)
{
	FExposureParams Params;
	Params.DarkThreshold = 0.15f;

	FExposureTracker Tracker;
	TestTrue(TEXT("Below threshold reveals"), HasReason(Tracker.Update(FVector::ZeroVector, 0.1f, 1.f, 0.2f, Params), ERevealReason::Dark));
	TestFalse(TEXT("At threshold does not"), HasReason(Tracker.Update(FVector::ZeroVector, 0.15f, 1.2f, 0.2f, Params), ERevealReason::Dark));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMansionExposureMonsterSightTest, "Mansion.Logic.Exposure.MonsterSight", MansionTestFlags)
bool FMansionExposureMonsterSightTest::RunTest(const FString& Parameters)
{
	FExposureParams Params;
	Params.MonsterSightLinger = 1.5f;
	Params.MonsterSightGap = 0.5f;

	FExposureTracker Tracker;
	Tracker.ReportMonsterSight(10.f, Params);
	TestTrue(TEXT("Seen right away"), HasReason(Tracker.Update(FVector::ZeroVector, 1.f, 10.f, 0.2f, Params), ERevealReason::SeenByMonster));
	TestTrue(TEXT("Still seen within linger"), HasReason(Tracker.Update(FVector::ZeroVector, 1.f, 11.4f, 0.2f, Params), ERevealReason::SeenByMonster));
	TestFalse(TEXT("Hidden after linger"), HasReason(Tracker.Update(FVector::ZeroVector, 1.f, 11.6f, 0.2f, Params), ERevealReason::SeenByMonster));

	// Voice Snip eligibility needs 3s of unbroken targeting.
	FExposureTracker Streak;
	for (float Time = 20.f; Time <= 23.01f; Time += 0.2f)
	{
		Streak.ReportMonsterSight(Time, Params);
	}
	TestTrue(TEXT("3s continuous sight"), Streak.GetContinuousMonsterSightTime(23.f, Params) >= 2.99f);

	Streak.ReportMonsterSight(24.f, Params); // 1s gap breaks the streak
	TestTrue(TEXT("Gap resets the streak"), Streak.GetContinuousMonsterSightTime(24.f, Params) < 0.01f);
	TestEqual(TEXT("Not currently seen means zero"), Streak.GetContinuousMonsterSightTime(30.f, Params), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMansionExposureForcedTest, "Mansion.Logic.Exposure.Forced", MansionTestFlags)
bool FMansionExposureForcedTest::RunTest(const FString& Parameters)
{
	FExposureParams Params;
	FExposureTracker Tracker;
	Tracker.ForcedRevealCount = 1;
	TestTrue(TEXT("Forced reveal"), HasReason(Tracker.Update(FVector::ZeroVector, 1.f, 1.f, 0.2f, Params), ERevealReason::Forced));
	Tracker.ForcedRevealCount = 0;
	TestFalse(TEXT("Released"), HasReason(Tracker.Update(FVector::ZeroVector, 1.f, 1.2f, 0.2f, Params), ERevealReason::Forced));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
