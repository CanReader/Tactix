// Copyright Sleak Software. All Rights Reserved.
//
// BT leaf task: evaluates all actions in a UTactixUtilityAsset against the
// current agent context, picks the highest scorer, writes its name to a
// Blackboard key, and fires a Blueprint event for project code to respond.
// Returns Succeeded (task is a snapshot — it doesn't run the action itself).

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "UTactixBTTask_RunUtility.generated.h"

class UTactixUtilityAsset;

UCLASS()
class TACTIXUE_API UTactixBTTask_RunUtility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTactixBTTask_RunUtility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

	// Asset holding the action + consideration definitions to evaluate.
	UPROPERTY(EditAnywhere, Category = "Tactix|Utility")
	TObjectPtr<UTactixUtilityAsset> UtilityAsset;

	// Name of the BB key (FName) where the winning action name is written.
	UPROPERTY(EditAnywhere, Category = "Tactix|Utility")
	FBlackboardKeySelector WinnerNameKey;

	// Optional BB key (float) for the winning action's score.
	UPROPERTY(EditAnywhere, Category = "Tactix|Utility")
	FBlackboardKeySelector WinnerScoreKey;

protected:
	// Override in Blueprint or subclass to act on the selected action.
	// ActionName is FName::None if no action scored above zero.
	UFUNCTION(BlueprintImplementableEvent, Category = "Tactix|Utility")
	void OnActionSelected(FName ActionName, float Score);
};
