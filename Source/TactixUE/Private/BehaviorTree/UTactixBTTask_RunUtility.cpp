// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTTask_RunUtility.cpp
 * @brief Implements the utility-selection BT task: snapshot the agent, pick the
 *        best action, publish it.
 */

#include "BehaviorTree/UTactixBTTask_RunUtility.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Components/UTactixAgentComponent.h"
#include "DataAssets/UTactixUtilityAsset.h"
#include "Controller/ATactixAIController.h"

UTactixBTTask_RunUtility::UTactixBTTask_RunUtility()
{
	NodeName = TEXT("Run Utility Selector");
	// This task is a pure snapshot — no ticking needed.
	bNotifyTick = false;
}

EBTNodeResult::Type UTactixBTTask_RunUtility::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                                          uint8* NodeMemory)
{
	if (!UtilityAsset) return EBTNodeResult::Failed;

	ATactixAIController* Controller = Cast<ATactixAIController>(OwnerComp.GetAIOwner());
	if (!Controller) return EBTNodeResult::Failed;

	UTactixAgentComponent* Agent = Controller->GetTactixAgent();
	if (!Agent) return EBTNodeResult::Failed;

	const Tactix::FTactixAgentContext Ctx = Agent->BuildContext();

	// Score every action and pick the winner.
	const int32 WinnerIdx = UtilityAsset->PickBestAction(Ctx);

	FName WinnerName = NAME_None;
	float WinnerScore = 0.0f;

	if (WinnerIdx != INDEX_NONE && UtilityAsset->Actions.IsValidIndex(WinnerIdx))
	{
		WinnerName  = UtilityAsset->Actions[WinnerIdx].ActionName;
		WinnerScore = UtilityAsset->ScoreAction(WinnerIdx, Ctx);
	}

	// Push results to Blackboard. Key types must be configured correctly by the designer.
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (!WinnerNameKey.SelectedKeyName.IsNone())
		{
			BB->SetValueAsName(WinnerNameKey.SelectedKeyName, WinnerName);
		}
		if (!WinnerScoreKey.SelectedKeyName.IsNone())
		{
			BB->SetValueAsFloat(WinnerScoreKey.SelectedKeyName, WinnerScore);
		}
	}

	// Let Blueprint / subclasses respond.
	OnActionSelected(WinnerName, WinnerScore);

	return EBTNodeResult::Succeeded;
}

FString UTactixBTTask_RunUtility::GetStaticDescription() const
{
	return FString::Printf(TEXT("Asset: %s"),
	    UtilityAsset ? *UtilityAsset->GetName() : TEXT("None"));
}
