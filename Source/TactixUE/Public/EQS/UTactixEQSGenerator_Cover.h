// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixEQSGenerator_Cover.h
 * @brief EQS generator that emits registered cover points as query items.
 *
 * Pulls every cover point from @ref UTactixWorldSubsystem's cover system and
 * emits its world position as an EQS point item. Pair it with
 * @ref UTactixEQSTest_Influence to score those points by influence, or with the
 * stock EQS distance/dot tests for ordinary scoring.
 */

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "UTactixEQSGenerator_Cover.generated.h"

/** @brief EQS generator: cover points from the world subsystem. */
UCLASS()
class TACTIXUE_API UTactixEQSGenerator_Cover : public UEnvQueryGenerator
{
	GENERATED_BODY()

public:
	UTactixEQSGenerator_Cover();

	/** @brief Emits one point item per registered cover point. */
	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;
	/** @brief Short editor title for the generator node. */
	virtual FText GetDescriptionTitle()  const override;
	/** @brief Longer editor description for the generator node. */
	virtual FText GetDescriptionDetails() const override;

	/** @brief When true, points already claimed by other agents are not emitted. */
	UPROPERTY(EditDefaultsOnly, Category = "Tactix|Cover")
	bool bExcludeClaimed = false;
};
