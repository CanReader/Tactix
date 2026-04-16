// Copyright Sleak Software. All Rights Reserved.
//
// BT decorator: passes when ATactixAIController has a valid active plan of the
// selected type (GOAP or HTN) with at least one unexecuted step remaining.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/UTactixBTTask_ExecutePlan.h"
#include "UTactixBTDecorator_PlanActive.generated.h"

UCLASS()
class TACTIXUE_API UTactixBTDecorator_PlanActive : public UBTDecorator
{
	GENERATED_BODY()

public:
	UTactixBTDecorator_PlanActive();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) const override;

	virtual FString GetStaticDescription() const override;

	UPROPERTY(EditAnywhere, Category = "Tactix|Plan")
	ETactixPlanType PlanType = ETactixPlanType::GOAP;
};
