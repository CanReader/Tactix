// Copyright Sleak Software. All Rights Reserved.
//
// EQS test: scores each item by sampling the UTactixWorldSubsystem influence
// map at the item's XY world position. High scores are "better" by default;
// set bInvertScore to penalise high-influence areas (e.g. danger channel).

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "UTactixEQSTest_Influence.generated.h"

UCLASS()
class TACTIXUE_API UTactixEQSTest_Influence : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UTactixEQSTest_Influence();

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle()   const override;
	virtual FText GetDescriptionDetails() const override;

	// Which channel to sample (0=Danger, 1=Control, 2=Resource, 3=Custom).
	UPROPERTY(EditDefaultsOnly, Category = "Tactix|Influence",
	          meta = (ClampMin = "0", ClampMax = "3"))
	int32 ChannelIndex = 0;

	// Flip the score so high influence = low desirability (use for danger).
	UPROPERTY(EditDefaultsOnly, Category = "Tactix|Influence")
	bool bInvertScore = false;

	// When true, use bilinear sampling (smoother scoring near cell boundaries).
	UPROPERTY(EditDefaultsOnly, Category = "Tactix|Influence")
	bool bBilinear = true;
};
