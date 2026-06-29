// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTTask_ExecutePlan.cpp
 * @brief Implements step-by-step plan execution for the BT task.
 *
 * @ref UTactixBTTask_ExecutePlan::StepPlan "StepPlan" is shared between the initial @c ExecuteTask and each
 * @c TickTask: it runs the next step of the chosen plan and reports whether the
 * plan is finished. Note @c AbortTask deliberately leaves the plan state alone so
 * a decorator higher up can decide whether to re-plan or resume.
 */

#include "BehaviorTree/UTactixBTTask_ExecutePlan.h"
#include "AIController.h"
#include "Controller/ATactixAIController.h"
#include "Components/UTactixAgentComponent.h"
#include "GOAP/ITactixGOAPAction.h"
#include "HTN/ITactixHTNTask.h"

UTactixBTTask_ExecutePlan::UTactixBTTask_ExecutePlan()
{
	NodeName    = TEXT("Execute Plan");
	bNotifyTick = true;
}

bool UTactixBTTask_ExecutePlan::StepPlan(UBehaviorTreeComponent& OwnerComp) const
{
	ATactixAIController* Controller = Cast<ATactixAIController>(OwnerComp.GetAIOwner());
	if (!Controller) return true; // signal "done" so the task fails out

	UTactixAgentComponent* Agent = Controller->GetTactixAgent();
	if (!Agent) return true;

	Tactix::FTactixAgentContext Ctx = Agent->BuildContext();

	if (PlanType == ETactixPlanType::GOAP)
	{
		if (!Controller->ActiveGOAPPlan.bValid) return true;
		const uint32 Step = Controller->GOAPPlanStep;
		if (Step >= Controller->ActiveGOAPPlan.Count)
		{
			Controller->bGOAPPlanActive = false;
			return true;
		}
		Controller->ActiveGOAPPlan.Steps[Step]->Execute(Ctx);
		Controller->GOAPPlanStep++;
		if (Controller->GOAPPlanStep >= Controller->ActiveGOAPPlan.Count)
		{
			Controller->bGOAPPlanActive = false;
			return true;
		}
		return false;
	}
	else // HTN
	{
		if (!Controller->ActiveHTNPlan.bValid) return true;
		const uint32 Step = Controller->HTNPlanStep;
		if (Step >= Controller->ActiveHTNPlan.Count)
		{
			Controller->bHTNPlanActive = false;
			return true;
		}
		Controller->ActiveHTNPlan.Steps[Step]->Execute(Ctx);
		Controller->HTNPlanStep++;
		if (Controller->HTNPlanStep >= Controller->ActiveHTNPlan.Count)
		{
			Controller->bHTNPlanActive = false;
			return true;
		}
		return false;
	}
}

EBTNodeResult::Type UTactixBTTask_ExecutePlan::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                                           uint8* NodeMemory)
{
	ATactixAIController* Controller = Cast<ATactixAIController>(OwnerComp.GetAIOwner());
	if (!Controller) return EBTNodeResult::Failed;

	const bool bGOAP = (PlanType == ETactixPlanType::GOAP);
	if (bGOAP && (!Controller->ActiveGOAPPlan.bValid || Controller->ActiveGOAPPlan.Count == 0))
		return EBTNodeResult::Failed;
	if (!bGOAP && (!Controller->ActiveHTNPlan.bValid || Controller->ActiveHTNPlan.Count == 0))
		return EBTNodeResult::Failed;

	const bool bDone = StepPlan(OwnerComp);
	return bDone ? EBTNodeResult::Succeeded : EBTNodeResult::InProgress;
}

void UTactixBTTask_ExecutePlan::TickTask(UBehaviorTreeComponent& OwnerComp,
                                         uint8* NodeMemory, float DeltaSeconds)
{
	const bool bDone = StepPlan(OwnerComp);
	if (bDone)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UTactixBTTask_ExecutePlan::AbortTask(UBehaviorTreeComponent& OwnerComp,
                                                         uint8* NodeMemory)
{
	// Leave plan state intact — a decorator can decide whether to re-plan.
	return EBTNodeResult::Aborted;
}

FString UTactixBTTask_ExecutePlan::GetStaticDescription() const
{
	return FString::Printf(TEXT("Execute %s Plan"),
	    PlanType == ETactixPlanType::GOAP ? TEXT("GOAP") : TEXT("HTN"));
}
