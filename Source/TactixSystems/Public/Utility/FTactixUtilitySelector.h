// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixUtilitySelector.h
 * @brief Picks the highest-scoring action, and computes a combined consideration
 *        score with compensation for the number of considerations.
 */

#pragma once

#include "TactixApi.h"
#include "Utility/ITactixAction.h"
#include "Utility/ITactixConsideration.h"

#include <cstddef>

namespace Tactix
{
	struct FTactixAgentContext;

	/** @brief Outcome of a @ref FTactixUtilitySelector::Pick call. */
	struct FTactixUtilityResult
	{
		/** @brief Sentinel @c Index value meaning "no action was chosen". */
		static constexpr std::size_t kNoWinner = ~std::size_t{0};

		std::size_t    Index{kNoWinner};  ///< Index of the winner in the input array, or @ref kNoWinner.
		float          Score{0.0f};       ///< Winning score; 0 when nothing was eligible.
		ITactixAction* Action{nullptr};   ///< The winning action, or null.

		/** @brief Whether a real winner was found. */
		bool HasWinner() const { return Index != kNoWinner && Action != nullptr; }
	};

	/** @brief Stateless helpers for utility-based action selection. */
	struct FTactixUtilitySelector
	{
		/**
		 * @brief Scores every eligible action and returns the best one.
		 *
		 * Skips null and ineligible (@ref ITactixAction::IsValid is false) actions.
		 * Strictly-greater comparison means ties resolve to the lower index, which
		 * keeps the choice stable from frame to frame when scores are equal.
		 *
		 * @param Actions Array of candidate actions; entries may be null.
		 * @param Count   Number of entries in @p Actions.
		 * @param Ctx     Read-only agent state passed to each action.
		 * @return The winner, or a result with @ref FTactixUtilityResult::HasWinner
		 *         false if nothing scored above zero.
		 */
		static FTactixUtilityResult Pick(ITactixAction* const* Actions, std::size_t Count,
		                                 const FTactixAgentContext& Ctx)
		{
			FTactixUtilityResult Best{};

			for (std::size_t i = 0; i < Count; ++i)
			{
				ITactixAction* A = Actions[i];
				if (A == nullptr || !A->IsValid(Ctx)) continue;

				const float Score = A->GetScore(Ctx);
				if (Score > Best.Score)
				{
					Best.Index  = i;
					Best.Score  = Score;
					Best.Action = A;
				}
			}

			return Best;
		}

		/**
		 * @brief Combines a set of considerations into one score in [0, 1].
		 *
		 * A plain product of considerations is biased: every extra factor below 1
		 * drags the result down, so an action with more considerations loses to one
		 * with fewer for no good reason. This applies the Mike Lewis make-up factor
		 * (from the IAUS literature) to counteract that bias, scaling each term back
		 * up based on how many considerations there are.
		 *
		 * @param Considerations Array of considerations; a null entry forces 0.
		 * @param Count          Number of considerations.
		 * @param Ctx            Read-only agent state.
		 * @return The compensated product in [0, 1]. Returns 0 immediately if any
		 *         consideration evaluates to 0 (a hard veto) or the input is empty.
		 */
		static float ScoreConsiderations(const ITactixConsideration* const* Considerations,
		                                 std::size_t Count, const FTactixAgentContext& Ctx)
		{
			if (Count == 0 || Considerations == nullptr) return 0.0f;

			const float ModFactor = 1.0f - (1.0f / static_cast<float>(Count));
			float Product = 1.0f;

			for (std::size_t i = 0; i < Count; ++i)
			{
				const ITactixConsideration* C = Considerations[i];
				if (C == nullptr) return 0.0f;

				float X = C->Evaluate(Ctx);
				if (X <= 0.0f) return 0.0f;
				if (X > 1.0f)  X = 1.0f;

				const float MakeUp = (1.0f - X) * ModFactor;
				X = X + (MakeUp * X);

				Product *= X;
			}

			return Product;
		}
	};
}
