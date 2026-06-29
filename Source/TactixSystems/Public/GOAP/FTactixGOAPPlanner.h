// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixGOAPPlanner.h
 * @brief A* planner over the GOAP world-state graph.
 */

#pragma once

#include "TactixApi.h"
#include "GOAP/FTactixWorldState.h"

#include <cstdint>

namespace Tactix
{
	struct FTactixAgentContext;
	class  FTactixArena;
	class  ITactixGOAPAction;

	/**
	 * @brief A computed plan: the ordered actions that get from start to goal.
	 *
	 * @warning @c Steps points into the arena passed to @ref FTactixGOAPPlanner::Plan.
	 *          It is valid only until that arena is reset or destroyed. Execute or
	 *          copy the plan before reusing the arena.
	 */
	struct FTactixGOAPPlan
	{
		ITactixGOAPAction** Steps{nullptr};  ///< Actions to run in order; arena-owned memory.
		uint32_t            Count{0};        ///< Number of steps.
		float               Cost{0.0f};      ///< Total cost of the plan (sum of action costs).
		bool                bValid{false};   ///< True if a plan was found. An empty valid plan means the goal already holds.
	};

	/** @brief Search limits for a GOAP plan. */
	struct FTactixGOAPConfig
	{
		uint32_t MaxNodes      = 512;  ///< Cap on expanded search nodes; planning fails if exceeded.
		uint32_t MaxPlanLength = 32;   ///< Cap on plan length; a longer solution is rejected.
	};

	/** @brief Stateless GOAP planner. All working memory comes from a caller arena. */
	class TACTIXSYSTEMS_API FTactixGOAPPlanner
	{
	public:
		/**
		 * @brief Runs A* from @p Start to @p Goal over the given actions.
		 *
		 * Every allocation, node pool, open-set heap, closed-set table, and the
		 * returned step array, comes out of @p Scratch, so the planner itself never
		 * touches the heap. The search is admissible (uses
		 * @ref FTactixWorldState::Distance), so the first plan found is optimal in
		 * total cost.
		 *
		 * @param Start       Current world state.
		 * @param Goal        Desired facts (its mask says which matter).
		 * @param Actions     Candidate actions; null entries are skipped.
		 * @param ActionCount Number of entries in @p Actions.
		 * @param Ctx         Agent context, forwarded to every action query.
		 * @param Scratch     Arena for all working memory and the result's steps.
		 * @param Config      Search limits.
		 * @return A plan. @c bValid is false if @p Goal is unreachable within the
		 *         limits or the arena runs out of space. A valid plan with
		 *         @c Count == 0 means @p Start already satisfies @p Goal.
		 */
		static FTactixGOAPPlan Plan(const FTactixWorldState&   Start,
		                            const FTactixWorldState&   Goal,
		                            ITactixGOAPAction* const*  Actions,
		                            uint32_t                   ActionCount,
		                            const FTactixAgentContext& Ctx,
		                            FTactixArena&              Scratch,
		                            FTactixGOAPConfig          Config = {});
	};
}
