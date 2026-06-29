// Copyright Sleak Software. All Rights Reserved.

/**
 * @file ITactixAction.h
 * @brief Interface for a Utility AI action: a thing the agent can choose to do,
 *        with a score and an execution body.
 */

#pragma once

#include "TactixApi.h"

namespace Tactix
{
	struct FTactixAgentContext;

	/**
	 * @brief One candidate behaviour the utility selector can pick.
	 *
	 * The selector asks each action whether it's currently possible
	 * (@ref IsValid), scores the possible ones (@ref GetScore), and runs the
	 * winner (@ref Execute). Scoring is read-only; only @ref Execute may act.
	 */
	class TACTIXSYSTEMS_API ITactixAction
	{
	public:
		virtual ~ITactixAction() = default;

		/**
		 * @brief How appealing this action is right now.
		 * @param Ctx Read-only agent state.
		 * @return A non-negative score; higher wins. Typically the product of the
		 *         action's considerations.
		 */
		virtual float GetScore(const FTactixAgentContext& Ctx) const = 0;

		/**
		 * @brief Whether the action is even eligible this frame.
		 * @param Ctx Read-only agent state.
		 * @return False to exclude the action from selection regardless of score.
		 */
		virtual bool  IsValid (const FTactixAgentContext& Ctx) const = 0;

		/**
		 * @brief Performs the action. Called only on the selected winner.
		 * @param Ctx Mutable agent state; this is the one place an action may act.
		 */
		virtual void  Execute (FTactixAgentContext& Ctx)            = 0;
	};
}
