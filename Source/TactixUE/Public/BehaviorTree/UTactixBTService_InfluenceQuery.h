// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTService_InfluenceQuery.h
 * @brief BT service that samples the world influence map under the agent each
 *        tick into a blackboard float.
 *
 * Every service tick it reads the agent's XY position, samples one channel of the
 * @ref UTactixWorldSubsystem influence map there, and writes the value to a
 * blackboard key. That key can feed a consideration input or be read directly in
 * Blueprint, e.g. to bias decisions away from danger.
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "UTactixBTService_InfluenceQuery.generated.h"

/** @brief Behavior Tree service: push an influence sample to the blackboard. */
UCLASS()
class TACTIXUE_API UTactixBTService_InfluenceQuery : public UBTService
{
	GENERATED_BODY()

public:
	UTactixBTService_InfluenceQuery();

	/** @brief Samples the influence map and writes the result each tick. */
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
	                      uint8* NodeMemory, float DeltaSeconds) override;

	/** @brief Editor-facing one-line summary of the node. */
	virtual FString GetStaticDescription() const override;

	/** @brief Channel to sample: 0 Danger, 1 Control, 2 Resource, 3 Custom. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Influence",
	          meta = (ClampMin = "0", ClampMax = "3"))
	int32 ChannelIndex = 0;

	/** @brief Blackboard Float key that receives the sampled value. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Influence")
	FBlackboardKeySelector InfluenceValueKey;

	/** @brief Use bilinear sampling (smoother, slightly costlier) instead of nearest. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Influence")
	bool bBilinear = true;
};
