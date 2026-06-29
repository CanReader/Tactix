// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTTask_ClaimCover.cpp
 * @brief Implements the cover query/claim BT task: build the query from agent and
 *        blackboard state, claim the winner, write it back.
 */

#include "BehaviorTree/UTactixBTTask_ClaimCover.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Controller/ATactixAIController.h"
#include "Components/UTactixAgentComponent.h"
#include "Subsystem/UTactixWorldSubsystem.h"

UTactixBTTask_ClaimCover::UTactixBTTask_ClaimCover()
{
	NodeName    = TEXT("Claim Cover");
	bNotifyTick = false;
}

EBTNodeResult::Type UTactixBTTask_ClaimCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                                          uint8* NodeMemory)
{
	ATactixAIController* Controller = Cast<ATactixAIController>(OwnerComp.GetAIOwner());
	if (!Controller) return EBTNodeResult::Failed;

	UTactixAgentComponent* Agent = Controller->GetTactixAgent();
	if (!Agent) return EBTNodeResult::Failed;

	UWorld* World = OwnerComp.GetWorld();
	if (!World) return EBTNodeResult::Failed;

	UTactixWorldSubsystem* TactixSub = World->GetSubsystem<UTactixWorldSubsystem>();
	if (!TactixSub) return EBTNodeResult::Failed;

	const Tactix::FTactixAgentContext Ctx = Agent->BuildContext();

	// Read threat location from Blackboard.
	Tactix::FTactixVec3 ThreatPos{};
	if (const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		const FVector BbThreat = BB->GetValueAsVector(ThreatLocationKey.SelectedKeyName);
		ThreatPos = { (float)BbThreat.X, (float)BbThreat.Y, (float)BbThreat.Z };
	}

	Tactix::FTactixCoverSystem<512>::FQuery Q{};
	Q.FromPosition   = Ctx.Position;
	Q.ThreatPosition = ThreatPos;
	Q.MaxRange       = MaxRange;
	Q.MinBlockDot    = MinBlockDot;
	Q.bExcludeClaimed = bExcludeClaimed;
	Q.Querier        = Ctx.AgentHandle;

	const auto CoverH = TactixSub->GetCoverSystem().QueryBest(Q);
	if (!CoverH.IsValid()) return EBTNodeResult::Failed;

	if (!TactixSub->GetCoverSystem().Claim(CoverH, Ctx.AgentHandle))
		return EBTNodeResult::Failed;

	// Mark the agent as in cover.
	Agent->bInCover = true;

	// Push the cover position to Blackboard if a key is configured.
	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (const Tactix::FTactixCoverPoint* P = TactixSub->GetCoverSystem().Get(CoverH))
		{
			if (!CoverPositionKey.SelectedKeyName.IsNone())
			{
				BB->SetValueAsVector(CoverPositionKey.SelectedKeyName,
				    FVector(P->Position.X, P->Position.Y, P->Position.Z));
			}
		}
	}

	return EBTNodeResult::Succeeded;
}

FString UTactixBTTask_ClaimCover::GetStaticDescription() const
{
	return FString::Printf(TEXT("MaxRange: %.0f  MinBlockDot: %.2f"), MaxRange, MinBlockDot);
}
