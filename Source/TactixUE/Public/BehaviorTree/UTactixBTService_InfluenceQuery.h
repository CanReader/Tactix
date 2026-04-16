// Copyright Sleak Software. All Rights Reserved.
//
// BT service: samples a channel of the world influence map at the agent's
// current XY position each tick and writes the value to a Blackboard key
// (float). Useful for consideration inputs — wire the BB key into a
// FTactixConsiderationConfig with InputType==Custom, or read it from Blueprint.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "UTactixBTService_InfluenceQuery.generated.h"

UCLASS()
class TACTIXUE_API UTactixBTService_InfluenceQuery : public UBTService
{
	GENERATED_BODY()

public:
	UTactixBTService_InfluenceQuery();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
	                      uint8* NodeMemory, float DeltaSeconds) override;

	virtual FString GetStaticDescription() const override;

	// Which influence map channel to sample (0=Danger, 1=Control, 2=Resource, 3=Custom).
	UPROPERTY(EditAnywhere, Category = "Tactix|Influence",
	          meta = (ClampMin = "0", ClampMax = "3"))
	int32 ChannelIndex = 0;

	// BB float key where the sampled value is written.
	UPROPERTY(EditAnywhere, Category = "Tactix|Influence")
	FBlackboardKeySelector InfluenceValueKey;

	// When true, use bilinear interpolation (slightly more expensive, smoother).
	UPROPERTY(EditAnywhere, Category = "Tactix|Influence")
	bool bBilinear = true;
};
