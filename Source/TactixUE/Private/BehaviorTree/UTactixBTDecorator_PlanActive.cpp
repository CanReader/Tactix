// Copyright Sleak Software. All Rights Reserved.

#include "BehaviorTree/UTactixBTDecorator_PlanActive.h"
#include "AIController.h"
#include "Controller/ATactixAIController.h"

UTactixBTDecorator_PlanActive::UTactixBTDecorator_PlanActive()
{
	NodeName = TEXT("Is Plan Active");
}

bool UTactixBTDecorator_PlanActive::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,
                                                                uint8* NodeMemory) const
{
	const ATactixAIController* Controller = Cast<ATactixAIController>(OwnerComp.GetAIOwner());
	if (!Controller) return false;

	if (PlanType == ETactixPlanType::GOAP)
	{
		return Controller->bGOAPPlanActive
		    && Controller->ActiveGOAPPlan.bValid
		    && Controller->GOAPPlanStep < Controller->ActiveGOAPPlan.Count;
	}
	else
	{
		return Controller->bHTNPlanActive
		    && Controller->ActiveHTNPlan.bValid
		    && Controller->HTNPlanStep < Controller->ActiveHTNPlan.Count;
	}
}

FString UTactixBTDecorator_PlanActive::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s plan has steps remaining"),
	    PlanType == ETactixPlanType::GOAP ? TEXT("GOAP") : TEXT("HTN"));
}
