// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixEQSTest_Influence.h
 * @brief EQS test that scores items by the influence map value beneath them.
 *
 * For each query item it samples one channel of the @ref UTactixWorldSubsystem
 * influence map at the item's XY position. By default higher influence scores
 * better; enable @ref UTactixEQSTest_Influence::bInvertScore "bInvertScore" to make high influence undesirable, which is
 * what you want for the danger channel (steer items toward safety).
 */

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "UTactixEQSTest_Influence.generated.h"

/** @brief EQS test: score items by influence map value. */
UCLASS()
class TACTIXUE_API UTactixEQSTest_Influence : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UTactixEQSTest_Influence();

	/** @brief Scores every item by its sampled influence value. */
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	/** @brief Short editor title for the test node. */
	virtual FText GetDescriptionTitle()   const override;
	/** @brief Longer editor description for the test node. */
	virtual FText GetDescriptionDetails() const override;

	/** @brief Channel to sample: 0 Danger, 1 Control, 2 Resource, 3 Custom. */
	UPROPERTY(EditDefaultsOnly, Category = "Tactix|Influence",
	          meta = (ClampMin = "0", ClampMax = "3"))
	int32 ChannelIndex = 0;

	/** @brief Flip scoring so high influence reads as low desirability (use for danger). */
	UPROPERTY(EditDefaultsOnly, Category = "Tactix|Influence")
	bool bInvertScore = false;

	/** @brief Use bilinear sampling (smoother near cell boundaries) instead of nearest. */
	UPROPERTY(EditDefaultsOnly, Category = "Tactix|Influence")
	bool bBilinear = true;
};
