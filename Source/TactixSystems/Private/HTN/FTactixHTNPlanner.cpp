// Copyright Sleak Software. All Rights Reserved.

#include "HTN/FTactixHTNPlanner.h"
#include "HTN/ITactixHTNTask.h"
#include "Foundation/TactixArena.h"

namespace Tactix
{
	namespace
	{
		struct FDecomposeCtx
		{
			FTactixWorldState*         State;
			ITactixHTNPrimitive**      Plan;
			uint32_t*                  PlanCount;
			uint32_t                   MaxPlan;
			const FTactixAgentContext* AgentCtx;
			uint32_t                   MaxDepth;
			uint32_t                   MaxSubs;
		};

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
