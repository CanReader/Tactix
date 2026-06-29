// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixEQSTest_Influence.cpp
 * @brief Implements the influence EQS test: sample the map per item and score it.
 */

#include "EQS/UTactixEQSTest_Influence.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "Subsystem/UTactixWorldSubsystem.h"

UTactixEQSTest_Influence::UTactixEQSTest_Influence()
{
	// Score-only mode by default; let the designer add a filter threshold.
	TestPurpose = EEnvTestPurpose::Score;
	Cost        = EEnvTestCost::Low;
	SetWorkOnFloatValues(true);
}

void UTactixEQSTest_Influence::RunTest(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner) return;

	UWorld* World = QueryOwner->GetWorld();
	if (!World) return;

	UTactixWorldSubsystem* TactixSub = World->GetSubsystem<UTactixWorldSubsystem>();
	if (!TactixSub) return;

	const Tactix::FTactixInfluenceMap<4>& Map = TactixSub->GetInfluenceMap();
	const std::size_t Ch = static_cast<std::size_t>(FMath::Clamp(ChannelIndex, 0, 3));

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLoc = QueryInstance.GetItemAsLocation(It.GetIndex());
		const Tactix::FTactixVec2 XY{ (float)ItemLoc.X, (float)ItemLoc.Y };

		float Score = bBilinear ? Map.SampleBilinear(Ch, XY) : Map.SampleNearest(Ch, XY);
		if (bInvertScore) Score = 1.0f - FMath::Clamp(Score, 0.0f, 1.0f);

		It.SetScore(TestPurpose, FilterType, Score, FloatValueMin.GetValue(), FloatValueMax.GetValue());
	}
}

FText UTactixEQSTest_Influence::GetDescriptionTitle() const
{
	static const TCHAR* Names[] = { TEXT("Danger"), TEXT("Control"), TEXT("Resource"), TEXT("Custom") };
	return FText::FromString(FString::Printf(TEXT("Tactix Influence [%s]"),
	    Names[FMath::Clamp(ChannelIndex, 0, 3)]));
}

FText UTactixEQSTest_Influence::GetDescriptionDetails() const
{
	return FText::FromString(bInvertScore ? TEXT("Inverted (lower influence = higher score)")
	                                      : TEXT("Direct (higher influence = higher score)"));
}
