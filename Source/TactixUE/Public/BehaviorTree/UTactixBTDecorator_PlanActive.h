// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTDecorator_PlanActive.h
 * @brief BT decorator that passes while a plan of the chosen type is running.
 *
 * Checks @ref ATactixAIController for a valid active plan of @ref UTactixBTDecorator_PlanActive::PlanType "PlanType" with at
 * least one step left to execute. Use it to guard an "execute plan" subtree so it
 * only runs while there's actually a plan to run.
 */

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/UTactixBTTask_ExecutePlan.h"
#include "UTactixBTDecorator_PlanActive.generated.h"

/** @brief Behavior Tree decorator: gate on having an active plan. */
UCLASS()
class TACTIXUE_API UTactixBTDecorator_PlanActive : public UBTDecorator
{
	GENERATED_BODY()

public:
	UTactixBTDecorator_PlanActive();

	/**
	 * @brief Evaluates the condition.
	 * @return True when the controller has an active @ref PlanType plan with steps
	 *         remaining.
	 */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) const override;

	/** @brief Editor-facing one-line summary of the node. */
	virtual FString GetStaticDescription() const override;

	/** @brief Which plan type to check for. */
	UPROPERTY(EditAnywhere, Category = "Tactix|Plan")
	ETactixPlanType PlanType = ETactixPlanType::GOAP;
};
