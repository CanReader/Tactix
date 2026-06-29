// Copyright Sleak Software. All Rights Reserved.

/**
 * @file ITactixGOAPAction.h
 * @brief Interface for a GOAP action: a graph edge with preconditions, effects
 *        and a cost.
 */

#pragma once

#include "TactixApi.h"
#include "GOAP/FTactixWorldState.h"

namespace Tactix
{
	struct FTactixAgentContext;

	/**
	 * @brief One operator the GOAP planner can chain to reach a goal.
	 *
	 * In the search, an action is a transition: it applies when the current state
	 * satisfies its preconditions, and following it overlays its effects onto the
	 * state at the given cost. Preconditions/effects/cost are queried fresh against
	 * the agent context, so the same action object can behave differently per agent.
	 */
	class TACTIXSYSTEMS_API ITactixGOAPAction
	{
	public:
		virtual ~ITactixGOAPAction() = default;

		/**
		 * @brief Facts that must hold for the action to be taken.
		 * @param Ctx Read-only agent state.
		 * @return A world state whose defined facts are the requirement.
		 */
		virtual FTactixWorldState GetPreconditions(const FTactixAgentContext& Ctx) const = 0;

		/**
		 * @brief Facts the action changes once taken.
		 * @param Ctx Read-only agent state.
		 * @return A world state whose defined facts are overlaid onto the state.
		 */
		virtual FTactixWorldState GetEffects      (const FTactixAgentContext& Ctx) const = 0;

		/**
		 * @brief Cost of taking the action.
		 * @param Ctx Read-only agent state.
		 * @return A non-negative cost. Negative values break A*'s admissibility and
		 *         the planner skips any edge that reports one.
		 */
		virtual float GetCost(const FTactixAgentContext& Ctx) const = 0;

		/**
		 * @brief Agent-level feasibility gate beyond the symbolic preconditions.
		 * @param Ctx Read-only agent state.
		 * @return False to exclude the action entirely. Use this for things the
		 *         boolean facts can't express: ammo counts, cooldowns, an animation
		 *         lock, line of sight.
		 */
		virtual bool IsApplicable(const FTactixAgentContext& Ctx) const = 0;

		/**
		 * @brief Runs the action at execution time.
		 * @param Ctx Mutable agent state.
		 * @note The real effects here must stay consistent with @ref GetEffects, or
		 *       the executed plan drifts from what the planner reasoned about.
		 */
		virtual void Execute(FTactixAgentContext& Ctx) = 0;

		/**
		 * @brief Human-readable name for debug overlays.
		 * @return A short label; defaults to the empty string.
		 */
		virtual const char* GetDebugName() const { return ""; }
	};
}
