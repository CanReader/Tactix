// Copyright Sleak Software. All Rights Reserved.

/**
 * @file ITactixHTNTask.h
 * @brief Task hierarchy for the HTN planner: the common base, primitive (leaf)
 *        tasks, and compound tasks that decompose into methods.
 *
 * An HTN ("hierarchical task network") plan is built by recursively breaking a
 * high-level task down until only primitive, directly-executable tasks remain. A
 * @ref Tactix::ITactixHTNCompound "ITactixHTNCompound" offers one or more methods (alternative
 * decompositions); the planner tries them in order and backtracks on failure. A
 * @ref Tactix::ITactixHTNPrimitive "ITactixHTNPrimitive" is a leaf that simply applies its effects and, at run
 * time, executes.
 */

#pragma once

#include "TactixApi.h"
#include "GOAP/FTactixWorldState.h"

#include <cstdint>

namespace Tactix
{
	struct FTactixAgentContext;

	/** @brief Base for every HTN task; only distinguishes primitive from compound. */
	class TACTIXSYSTEMS_API ITactixHTNTask
	{
	public:
		virtual ~ITactixHTNTask() = default;

		/**
		 * @brief Which kind of task this is.
		 * @return True for a primitive (leaf); false for a compound (decomposable).
		 *         The planner uses this to decide whether to execute or expand, and
		 *         to choose the safe downcast.
		 */
		virtual bool IsPrimitive() const = 0;
	};

	/** @brief A leaf task: directly executable, with planner-visible effects. */
	class TACTIXSYSTEMS_API ITactixHTNPrimitive : public ITactixHTNTask
	{
	public:
		/**
		 * @brief Whether this task can run given the current planning state.
		 * @param State Symbolic state at this point in the decomposition.
		 * @param Ctx   Read-only agent state.
		 * @return False to make the planner backtrack out of the current method.
		 */
		virtual bool IsApplicable(const FTactixWorldState& State,
		                          const FTactixAgentContext& Ctx) const = 0;

		/**
		 * @brief Facts this task changes, applied to the planning state.
		 * @param Ctx Read-only agent state.
		 * @return The effect overlay.
		 * @note The runtime @ref Execute may do more, but must keep these bits
		 *       honest or later tasks plan against a state that won't actually hold.
		 */
		virtual FTactixWorldState GetEffects(const FTactixAgentContext& Ctx) const = 0;

		/**
		 * @brief Runs the task at execution time.
		 * @param Ctx Mutable agent state.
		 */
		virtual void Execute(FTactixAgentContext& Ctx) = 0;
	};

	/** @brief A task that decomposes into subtasks via one of several methods. */
	class TACTIXSYSTEMS_API ITactixHTNCompound : public ITactixHTNTask
	{
	public:
		/** @brief How many alternative methods this task offers. */
		virtual uint32_t GetMethodCount() const = 0;

		/**
		 * @brief Whether a given method is usable in the current state.
		 * @param MethodIndex Method to test, in `[0, GetMethodCount())`.
		 * @param State       Symbolic state at this point.
		 * @param Ctx         Read-only agent state.
		 * @return True if the planner may try expanding this method.
		 */
		virtual bool MethodApplies(uint32_t MethodIndex,
		                           const FTactixWorldState& State,
		                           const FTactixAgentContext& Ctx) const = 0;

		/**
		 * @brief Writes a method's subtasks into a caller buffer.
		 * @param MethodIndex Method to expand.
		 * @param OutSubtasks Destination array.
		 * @param MaxCount    Capacity of @p OutSubtasks.
		 * @return Number of subtasks written.
		 * @note The planner tries methods in index order, so order them by
		 *       preference, best method first. The subtasks are decomposed left to
		 *       right and the method fails (triggering backtrack) if any subtask does.
		 */
		virtual uint32_t GetMethodSubtasks(uint32_t          MethodIndex,
		                                   ITactixHTNTask**  OutSubtasks,
		                                   uint32_t          MaxCount) const = 0;
	};
}
