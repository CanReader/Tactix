// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixHTNPlanner.cpp
 * @brief Recursive decomposition for @ref Tactix::FTactixHTNPlanner.
 */

#include "HTN/FTactixHTNPlanner.h"
#include "HTN/ITactixHTNTask.h"
#include "Foundation/TactixArena.h"

namespace Tactix
{
	namespace
	{
		/** @brief Mutable state threaded through the recursive @ref Decompose. */
		struct FDecomposeCtx
		{
			FTactixWorldState*         State;     ///< Evolving planning state (effects applied as primitives are accepted).
			ITactixHTNPrimitive**      Plan;      ///< Output plan buffer being filled.
			uint32_t*                  PlanCount; ///< Number of primitives written so far.
			uint32_t                   MaxPlan;   ///< Capacity of @c Plan.
			const FTactixAgentContext* AgentCtx;  ///< Agent context for task queries.
			uint32_t                   MaxDepth;  ///< Recursion depth ceiling.
			uint32_t                   MaxSubs;   ///< Per-method subtask ceiling.
		};

		/**
		 * @brief Decomposes one task into the plan, recursing through compounds.
		 *
		 * Primitives that are applicable append themselves and apply their effects.
		 * Compounds try each applicable method in order; before a method's subtasks
		 * are expanded the state and plan length are snapshotted, and if any subtask
		 * fails both are restored so the next method starts clean.
		 *
		 * @param Task  Task to decompose. Null fails.
		 * @param C     Shared decomposition context.
		 * @param Depth Current recursion depth; failure once it reaches @c MaxDepth.
		 * @return True if @p Task (and its whole subtree) decomposed successfully.
		 */
		bool Decompose(ITactixHTNTask* Task, FDecomposeCtx& C, uint32_t Depth)
		{
			if (Task == nullptr)         return false;
			if (Depth >= C.MaxDepth)     return false;

			if (Task->IsPrimitive())
			{
				auto* P = static_cast<ITactixHTNPrimitive*>(Task);
				if (!P->IsApplicable(*C.State, *C.AgentCtx)) return false;
				if (*C.PlanCount >= C.MaxPlan)               return false;

				C.Plan[(*C.PlanCount)++] = P;
				C.State->Apply(P->GetEffects(*C.AgentCtx));
				return true;
			}

			auto* Cmp = static_cast<ITactixHTNCompound*>(Task);
			const uint32_t Methods = Cmp->GetMethodCount();

			// Subtask buffer on the call stack; bounded by MaxSubs.
			// Fixed at a sane upper limit to avoid VLAs (MSVC doesn't support them).
			constexpr uint32_t kHardCap = 32;
			ITactixHTNTask* Subs[kHardCap] = {nullptr};
			const uint32_t Cap = C.MaxSubs < kHardCap ? C.MaxSubs : kHardCap;

			for (uint32_t m = 0; m < Methods; ++m)
			{
				if (!Cmp->MethodApplies(m, *C.State, *C.AgentCtx)) continue;

				const FTactixWorldState SnapState = *C.State;
				const uint32_t          SnapPlan  = *C.PlanCount;

				const uint32_t N = Cmp->GetMethodSubtasks(m, Subs, Cap);
				if (N > Cap) continue;

				bool bOk = true;
				for (uint32_t i = 0; i < N; ++i)
				{
					if (!Decompose(Subs[i], C, Depth + 1)) { bOk = false; break; }
				}
				if (bOk) return true;

				*C.State     = SnapState;
				*C.PlanCount = SnapPlan;
			}
			return false;
		}
	}

	FTactixHTNPlan FTactixHTNPlanner::Plan(
		ITactixHTNTask*            Root,
		const FTactixWorldState&   InitialState,
		const FTactixAgentContext& Ctx,
		FTactixArena&              Scratch,
		FTactixHTNConfig           Config)
	{
		FTactixHTNPlan Result{};

		if (Root == nullptr || Config.MaxPlanLength == 0) return Result;

		ITactixHTNPrimitive** Plan = Scratch.EmplaceArrayUninitialised<ITactixHTNPrimitive*>(Config.MaxPlanLength);
		if (Plan == nullptr) return Result;

		FTactixWorldState State = InitialState;
		uint32_t          Count = 0;

		FDecomposeCtx C{
			&State, Plan, &Count,
			Config.MaxPlanLength, &Ctx,
			Config.MaxRecursionDepth,
			Config.MaxMethodSubtasks
		};

		if (!Decompose(Root, C, 0)) return Result;

		Result.Steps  = Plan;
		Result.Count  = Count;
		Result.bValid = true;
		return Result;
	}
}
