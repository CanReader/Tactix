// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTDecorator_InCover.cpp
 * @brief Implements the in-cover decorator condition.
 */

#include "BehaviorTree/UTactixBTDecorator_InCover.h"
#include "AIController.h"
#include "Controller/ATactixAIController.h"
#include "Components/UTactixAgentComponent.h"

UTactixBTDecorator_InCover::UTactixBTDecorator_InCover()
{
	NodeName = TEXT("Is In Cover");
}

bool UTactixBTDecorator_InCover::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,
                                                             uint8* NodeMemory) const
{
	const ATactixAIController* Controller = Cast<ATactixAIController>(OwnerComp.GetAIOwner());
	if (!Controller) return false;

	const UTactixAgentComponent* Agent = Controller->GetTactixAgent();
	return Agent && Agent->bInCover;
}

FString UTactixBTDecorator_InCover::GetStaticDescription() const
{
	return TEXT("Agent bInCover == true");
}
