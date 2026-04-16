// Copyright Sleak Software. All Rights Reserved.
//
// Designer-facing data asset: a named set of scored actions for one AI
// behaviour context (e.g. "CombatActions"). Assign it on a
// UTactixBTTask_RunUtility node. Each action gets a list of considerations;
// the task evaluates them with Mike Lewis compensation and picks the winner.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TactixUETypes.h"
#include "Foundation/TactixCurve.h"
#include "Agent/FTactixAgentContext.h"
#include "UTactixUtilityAsset.generated.h"

UCLASS(BlueprintType)
class TACTIXUE_API UTactixUtilityAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	TArray<FTactixActionConfig> Actions;

	// Score one action by index. Returns 0 if Index is out of range or any
	// consideration evaluates to zero (short-circuit). Lewis compensation
	// is applied over the full consideration list.
	float ScoreAction(int32 Index, const Tactix::FTactixAgentContext& Ctx) const;

	// Score all actions and return the index of the highest scorer (-1 if none
	// exceed zero). Writes per-action scores to OutScores if non-null.
	int32 PickBestAction(const Tactix::FTactixAgentContext& Ctx,
	                     TArray<float>* OutScores = nullptr) const;

private:
	static Tactix::FTactixCurve MakeCurve(const FTactixConsiderationConfig& Cfg);
	static float                GetRawInput(const FTactixConsiderationConfig& Cfg,
	                                        const Tactix::FTactixAgentContext& Ctx);
};
