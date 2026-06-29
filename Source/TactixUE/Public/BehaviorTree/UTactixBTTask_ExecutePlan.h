// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTTask_ExecutePlan.h
 * @brief BT task that walks the controller's active GOAP or HTN plan, one step
 *        per tick.
 *
 * Reads the plan stored on @ref ATactixAIController (placed there by a planning
 * task), executes one step each tick via @c TickTask, and finishes once the plan
 * is exhausted. It runs latently rather than all at once so each step can take
 * real game time.
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UTactixBTTask_ExecutePlan.generated.h"

/** @brief Selects which planner's active plan a node operates on. */
UENUM(BlueprintType)
enum class ETactixPlanType : uint8
{
	GOAP UMETA(DisplayName = "GOAP"),  ///< The controller's @c ActiveGOAPPlan.
	HTN  UMETA(DisplayName = "HTN"),   ///< The controller's @c ActiveHTNPlan.
};

/** @brief Behavior Tree task: execute the active plan step by step. */
UCLASS()
class TACTIXUE_API UTactixBTTask_ExecutePlan : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTactixBTTask_ExecutePlan();

	/**
	 * @brief Begins execution.
	 * @return InProgress while the plan runs, Failed if there's no valid plan.
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) override;
	/** @brief Marks the active plan inactive when the task is aborted. */
	virtual EBTNodeResult::Type AbortTask (UBehaviorTreeComponent& OwnerComp,
	                                       uint8* NodeMemory) override;
	/** @brief Advances the plan by one step; finishes the task when exhausted. */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp,
	                      uint8* NodeMemory, float DeltaSeconds) override;

	/** @brief Editor-facing one-line summary of the node. */
	virtual FString GetStaticDescription() const override;

	/** @brief Which plan (GOAP or HTN) to execute. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Plan")
	ETactixPlanType PlanType = ETactixPlanType::GOAP;

private:
	/**
	 * @brief Runs the next step of the selected plan.
	 * @return True when the plan has no steps left (i.e. it just finished).
	 */
	bool StepPlan(UBehaviorTreeComponent& OwnerComp) const;
};
