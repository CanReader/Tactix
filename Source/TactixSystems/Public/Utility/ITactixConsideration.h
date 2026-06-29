// Copyright Sleak Software. All Rights Reserved.

/**
 * @file ITactixConsideration.h
 * @brief Interface for a single Utility AI consideration: one normalised factor
 *        in an action's overall score.
 */

#pragma once

#include "TactixApi.h"

namespace Tactix
{
	struct FTactixAgentContext;

	/**
	 * @brief One scoring factor evaluated against an agent's current state.
	 *
	 * An action's appeal is the product of its considerations, so each one answers
	 * a narrow question ("how low is my health?", "how close is the threat?") and
	 * returns a normalised weight. Implementations must be pure: scoring may never
	 * change the agent.
	 */
	class TACTIXSYSTEMS_API ITactixConsideration
	{
	public:
		virtual ~ITactixConsideration() = default;

		/**
		 * @brief Scores this consideration for the given agent snapshot.
		 * @param Ctx Read-only agent state.
		 * @return A value in [0, 1]. The selector multiplies considerations, so a
		 *         return of 0 vetoes the action outright and values must stay in
		 *         range or the product is meaningless.
		 */
		virtual float Evaluate(const FTactixAgentContext& Ctx) const = 0;
	};
}
