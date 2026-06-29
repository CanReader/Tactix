// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixBTService_InfluenceQuery.cpp
 * @brief Implements the influence-sampling BT service.
 *
 * The default tick interval is a quarter second (with a little jitter) rather
 * than every frame: spatial awareness doesn't need per-frame resolution and the
 * jitter spreads the cost of many agents across frames.
 */

#include "BehaviorTree/UTactixBTService_InfluenceQuery.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Controller/ATactixAIController.h"
#include "Components/UTactixAgentComponent.h"
#include "Subsystem/UTactixWorldSubsystem.h"

UTactixBTService_InfluenceQuery::UTactixBTService_InfluenceQuery()
{
	NodeName = TEXT("Influence Query");
	// Default 0.25 s interval — enough for spatial awareness, not every frame.
	Interval      = 0.25f;
	RandomDeviation = 0.05f;
}

void UTactixBTService_InfluenceQuery::TickNode(UBehaviorTreeComponent& OwnerComp,
                                               uint8* NodeMemory, float DeltaSeconds)
{
	UWorld* World = OwnerComp.GetWorld();
	if (!World) return;

	UTactixWorldSubsystem* TactixSub = World->GetSubsystem<UTactixWorldSubsystem>();
	if (!TactixSub) return;

	const ATactixAIController* Controller = Cast<ATactixAIController>(OwnerComp.GetAIOwner());
	if (!Controller) return;

	const UTactixAgentComponent* Agent = Controller->GetTactixAgent();
	if (!Agent) return;

	const Tactix::FTactixAgentContext Ctx = Agent->BuildContext();
	const Tactix::FTactixVec2 XY{ Ctx.Position.X, Ctx.Position.Y };

	const Tactix::FTactixInfluenceMap<4>& Map = TactixSub->GetInfluenceMap();
	const std::size_t Ch = static_cast<std::size_t>(FMath::Clamp(ChannelIndex, 0, 3));

	const float Value = bBilinear ? Map.SampleBilinear(Ch, XY) : Map.SampleNearest(Ch, XY);

	if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
	{
		if (InfluenceValueKey.IsSet())
		{
			BB->SetValueAsFloat(InfluenceValueKey.SelectedKeyName, Value);
		}
	}
}

FString UTactixBTService_InfluenceQuery::GetStaticDescription() const
{
	static const TCHAR* ChannelNames[] = { TEXT("Danger"), TEXT("Control"), TEXT("Resource"), TEXT("Custom") };
	const int32 Ch = FMath::Clamp(ChannelIndex, 0, 3);
	return FString::Printf(TEXT("Channel %d (%s) → BB:%s"),
	    Ch, ChannelNames[Ch], *InfluenceValueKey.SelectedKeyName.ToString());
}
