// Copyright Sleak Software. All Rights Reserved.
//
// BT task that advances the active GOAP or HTN plan stored on
// ATactixAIController one step per tick. Returns Succeeded when the plan is
// exhausted, Failed if no valid plan exists.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UTactixBTTask_ExecutePlan.generated.h"

UENUM(BlueprintType)
enum class ETactixPlanType : uint8
{
	GOAP UMETA(DisplayName = "GOAP"),
	HTN  UMETA(DisplayName = "HTN"),
};

UCLASS()
class TACTIXUE_API UTactixBTTask_ExecutePlan : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTactixBTTask_ExecutePlan();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask (UBehaviorTreeComponent& OwnerComp,
	                                       uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp,
	                      uint8* NodeMemory, float DeltaSeconds) override;

	virtual FString GetStaticDescription() const override;

	// Which planner's active plan to execute.
	UPROPERTY(EditAnywhere, Category = "Tactix|Plan")
	ETactixPlanType PlanType = ETactixPlanType::GOAP;

private:
	// Execute one step of the current plan. Returns true when the plan finishes.
	bool StepPlan(UBehaviorTreeComponent& OwnerComp) const;
};
