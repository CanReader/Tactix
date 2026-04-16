// Copyright Sleak Software. All Rights Reserved.
//
// BT task: queries the world cover system for the best available cover point
// relative to the agent and a threat position, claims it, sets the agent's
// bInCover flag, and writes the cover world position to an optional BB key.
// Returns Failed if no suitable point is found.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "UTactixBTTask_ClaimCover.generated.h"

UCLASS()
class TACTIXUE_API UTactixBTTask_ClaimCover : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UTactixBTTask_ClaimCover();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	                                        uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

	// BB vector key holding the threat/enemy world position.
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover")
	FBlackboardKeySelector ThreatLocationKey;

	// Optional BB vector key that receives the claimed cover point's position.
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover")
	FBlackboardKeySelector CoverPositionKey;

	// Search radius around the agent for candidate cover points.
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover", meta = (ClampMin = "0.0"))
	float MaxRange = 2000.0f;

	// Minimum blocking dot: 0 = any direction, 0.5 = must block within 60 deg.
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinBlockDot = 0.3f;

	// If true, skip points already claimed by other agents.
	UPROPERTY(EditAnywhere, Category = "Tactix|Cover")
	bool bExcludeClaimed = true;
};
