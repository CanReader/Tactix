// Copyright Sleak Software. All Rights Reserved.
//
// BT decorator: passes when the controlled pawn's UTactixAgentComponent
// reports bInCover == true. Invert the condition via the standard BT inverse
// checkbox to branch on "not in cover".

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "UTactixBTDecorator_InCover.generated.h"

UCLASS()
class TACTIXUE_API UTactixBTDecorator_InCover : public UBTDecorator
{
	GENERATED_BODY()

public:
	UTactixBTDecorator_InCover();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) const override;

	virtual FString GetStaticDescription() const override;
};
