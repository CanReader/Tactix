// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTTask_RunUtility.h
 * @brief BT leaf that runs one utility-selection pass and reports the winner.
 *
 * The task scores every action in its @ref UTactixUtilityAsset against the
 * agent's current context, picks the highest scorer, writes the winner to the
 * blackboard, and fires @ref UTactixBTTask_RunUtility::OnActionSelected "OnActionSelected" for Blueprint/subclass code to act
 * on. It is a decision step, not an execution step: it never runs the action
 * itself, and always returns Succeeded.
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "UTactixBTTask_RunUtility.generated.h"

class UTactixUtilityAsset;

/** @brief Behavior Tree task: select the best action from a utility asset. */
UCLASS()
class TACTIXUE_API UTactixBTTask_RunUtility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTactixBTTask_RunUtility();

	/**
	 * @brief Scores the asset's actions, writes the winner, fires the event.
	 * @return Succeeded always (Failed only on a missing agent/asset).
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) override;

	/** @brief Editor-facing one-line summary of the node. */
	virtual FString GetStaticDescription() const override;

	/** @brief The actions and considerations to evaluate. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Utility")
	TObjectPtr<UTactixUtilityAsset> UtilityAsset;

	/** @brief Blackboard Name key that receives the winning action's name. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Utility")
	FBlackboardKeySelector WinnerNameKey;

	/** @brief Optional blackboard Float key that receives the winning score. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Utility")
	FBlackboardKeySelector WinnerScoreKey;

protected:
	/**
	 * @brief Hook fired with the chosen action; implement in Blueprint or a subclass.
	 * @param ActionName Winning action, or @c FName::None if nothing scored above zero.
	 * @param Score      The winning score (0 when there's no winner).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tactix|Utility")
	void OnActionSelected(FName ActionName, float Score);
};
