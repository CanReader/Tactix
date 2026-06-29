// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTDecorator_InCover.h
 * @brief BT decorator that passes while the agent is in cover.
 *
 * Reads @c bInCover from the controlled pawn's @ref UTactixAgentComponent. Use
 * the standard Behavior Tree "inverse condition" checkbox to branch on
 * "not in cover" instead.
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "UTactixBTDecorator_InCover.generated.h"

/** @brief Behavior Tree decorator: gate on the agent's cover state. */
UCLASS()
class TACTIXUE_API UTactixBTDecorator_InCover : public UBTDecorator
{
	GENERATED_BODY()

public:
	UTactixBTDecorator_InCover();

	/**
	 * @brief Evaluates the condition.
	 * @return True when the agent component reports @c bInCover.
	 */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) const override;

	/** @brief Editor-facing one-line summary of the node. */
	virtual FString GetStaticDescription() const override;
};
