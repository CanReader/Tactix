// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixHTNPlanner.h
 * @brief Depth-first HTN planner with method-order fallback and backtracking.
 */

#pragma once

#include "TactixApi.h"
#include "GOAP/FTactixWorldState.h"

#include <cstdint>

namespace Tactix
{
	struct FTactixAgentContext;
	class  FTactixArena;
	class  ITactixHTNTask;
	class  ITactixHTNPrimitive;

	/**
	 * @brief A fully decomposed HTN plan: a flat, ordered list of primitives.
	 *
	 * @warning @c Steps points into the arena passed to @ref FTactixHTNPlanner::Plan
	 *          and is only valid until that arena is reset. Execute or copy first.
	 */
	struct FTactixHTNPlan
	{
		ITactixHTNPrimitive** Steps{nullptr};  ///< Primitive tasks to run in order; arena-owned.
		uint32_t              Count{0};        ///< Number of steps.
		bool                  bValid{false};   ///< True if the root decomposed successfully.
	};

	/** @brief Limits that bound HTN decomposition. */
	struct FTactixHTNConfig
	{
		uint32_t MaxPlanLength      = 32;  ///< Cap on emitted primitives; decomposition fails past it.
		uint32_t MaxRecursionDepth  = 32;  ///< Cap on decomposition depth; guards against runaway recursion.
		uint32_t MaxMethodSubtasks  = 16;  ///< Cap on subtasks read from one method per expansion.
	};

	/** @brief Stateless HTN planner. Working memory comes from a caller arena. */
	class TACTIXSYSTEMS_API FTactixHTNPlanner
	{
	public:
		/**
		 * @brief Decomposes @p Root into a plan of primitives.
		 *
		 * Walks the task tree depth-first. At each compound task it tries the
		 * applicable methods in index order; if a method's subtasks fail to fully
		 * decompose, it rolls the planning state and the partial plan back to the
		 * snapshot taken before that method and tries the next one. The plan's step
		 * array is allocated from @p Scratch.
		 *
		 * @param Root         Top-level task to decompose. Null yields an invalid plan.
		 * @param InitialState Starting symbolic state.
		 * @param Ctx          Agent context forwarded to task queries.
		 * @param Scratch      Arena for the result's step array.
		 * @param Config       Decomposition limits.
		 * @return A plan with @c bValid true on success; false if no method chain
		 *         decomposes within the limits or the arena runs out.
		 */
		static FTactixHTNPlan Plan(ITactixHTNTask*            Root,
		                           const FTactixWorldState&   InitialState,
		                           const FTactixAgentContext& Ctx,
		                           FTactixArena&              Scratch,
		                           FTactixHTNConfig           Config = {});
	};
}
