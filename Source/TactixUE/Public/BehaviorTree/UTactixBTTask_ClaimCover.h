// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTTask_ClaimCover.h
 * @brief BT task that finds, claims and records the best cover for the agent.
 *
 * Queries @ref UTactixWorldSubsystem's cover system for the best point given the
 * agent's position and a threat location read from the blackboard, claims it for
 * the agent, sets the agent's @c bInCover flag, and optionally writes the cover
 * position back to the blackboard for a subsequent move. Returns Failed when no
 * point qualifies.
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "UTactixBTTask_ClaimCover.generated.h"

/** @brief Behavior Tree task: query and claim the best cover point. */
UCLASS()
class TACTIXUE_API UTactixBTTask_ClaimCover : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTactixBTTask_ClaimCover();

	/**
	 * @brief Runs the cover query and claims the winner.
	 * @return Succeeded if a point was claimed, Failed otherwise.
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) override;

	/** @brief Editor-facing one-line summary of the node. */
	virtual FString GetStaticDescription() const override;

	/** @brief Blackboard Vector key holding the threat/enemy world position. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover")
	FBlackboardKeySelector ThreatLocationKey;

	/** @brief Optional blackboard Vector key that receives the claimed point's position. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover")
	FBlackboardKeySelector CoverPositionKey;

	/** @brief Search radius around the agent for candidate cover. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover", meta = (ClampMin = "0.0"))
	float MaxRange = 2000.0f;

	/** @brief Minimum blocking quality; 0 accepts any blocking angle, 0.5 is a ~60 degree wedge. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinBlockDot = 0.3f;

	/** @brief When true, skip points already claimed by other agents. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover")
	bool bExcludeClaimed = true;
};
