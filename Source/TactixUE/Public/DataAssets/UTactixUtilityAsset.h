// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixUtilityAsset.h
 * @brief Designer-facing asset: a named set of scored actions for one decision
 *        context.
 *
 * Author one of these per behaviour context (say "CombatActions") and assign it
 * to a @ref UTactixBTTask_RunUtility node. Each action lists its considerations;
 * scoring multiplies them with Mike Lewis compensation (see
 * @ref Tactix::FTactixUtilitySelector::ScoreConsiderations) and the highest
 * scorer wins. This asset both stores the config and performs the scoring,
 * translating the Blueprint-side @ref FTactixConsiderationConfig into the core
 * curve/input types.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TactixUETypes.h"
#include "Foundation/TactixCurve.h"
#include "Agent/FTactixAgentContext.h"
#include "UTactixUtilityAsset.generated.h"

/** @brief Data asset holding scored actions and the logic to evaluate them. */
UCLASS(BlueprintType)
class TACTIXUE_API UTactixUtilityAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** @brief The actions this asset can choose between. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	TArray<FTactixActionConfig> Actions;

	/**
	 * @brief Scores a single action.
	 * @param Index Action index into @ref Actions.
	 * @param Ctx   Agent snapshot to score against.
	 * @return The compensated consideration product in [0, 1], or 0 if @p Index is
	 *         out of range or any consideration vetoes (evaluates to 0).
	 */
	float ScoreAction(int32 Index, const Tactix::FTactixAgentContext& Ctx) const;

	/**
	 * @brief Scores all actions and returns the best.
	 * @param Ctx       Agent snapshot to score against.
	 * @param OutScores Optional; if non-null, receives every action's score in order.
	 * @return Index of the highest scorer, or -1 if none scored above zero.
	 */
	int32 PickBestAction(const Tactix::FTactixAgentContext& Ctx,
	                     TArray<float>* OutScores = nullptr) const;

private:
	/** @brief Builds a core response curve from a Blueprint consideration config. */
	static Tactix::FTactixCurve MakeCurve(const FTactixConsiderationConfig& Cfg);
	/** @brief Reads and normalises a consideration's raw input from the context. */
	static float                GetRawInput(const FTactixConsiderationConfig& Cfg,
	                                        const Tactix::FTactixAgentContext& Ctx);
};
