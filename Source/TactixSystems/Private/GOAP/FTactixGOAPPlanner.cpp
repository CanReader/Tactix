// Copyright Sleak Software. All Rights Reserved.

#include "GOAP/FTactixGOAPPlanner.h"
#include "GOAP/ITactixGOAPAction.h"
#include "Foundation/TactixArena.h"

namespace Tactix
{
	namespace
	{
		struct FNode
		{
			FTactixWorldState State;
			int32_t           Parent;
			int32_t           ActionIdx;
			float             G;
			float             F;
		};

		struct FOpenSet
		{
			int32_t*     Heap;
			uint32_t     Size;
			uint32_t     Capacity;
			const FNode* Nodes;

			bool Push(int32_t Idx)
			{
				if (Size >= Capacity) return false;
				uint32_t i = Size++;
				Heap[i] = Idx;
				while (i > 0)
				{
					const uint32_t p = (i - 1) >> 1;
					if (Nodes[Heap[p]].F <= Nodes[Heap[i]].F) break;
					const int32_t t = Heap[p]; Heap[p] = Heap[i]; Heap[i] = t;
					i = p;
				}
				return true;
			}

			int32_t Pop()
			{
				if (Size == 0) return -1;
				const int32_t Top = Heap[0];
				Heap[0] = Heap[--Size];
				uint32_t i = 0;
				while (true)
				{
					const uint32_t l = 2u * i + 1u;
					const uint32_t r = 2u * i + 2u;
					uint32_t m = i;
					if (l < Size && Nodes[Heap[l]].F < Nodes[Heap[m]].F) m = l;
					if (r < Size && Nodes[Heap[r]].F < Nodes[Heap[m]].F) m = r;
					if (m == i) break;
					const int32_t t = Heap[i]; Heap[i] = Heap[m]; Heap[m] = t;
					i = m;
				}
				return Top;
			}
		};

		// Linear-probing table: each slot stores a node index, -1 means empty.
		// Keyed on state value+mask; same state overwrites when we find a cheaper G.
		struct FClosedSet
		{
			int32_t*     Slots;
			uint32_t     Capacity;
			const FNode* Nodes;

			int32_t Find(const FTactixWorldState& S) const
			{
				const uint64_t H    = S.Hash();
				const uint32_t Mask = Capacity - 1;
				for (uint32_t p = 0; p < Capacity; ++p)
				{
					const uint32_t Slot = static_cast<uint32_t>((H + p) & Mask);
					const int32_t  Idx  = Slots[Slot];
					if (Idx < 0) return -1;
					if (Nodes[Idx].State == S) return Idx;
				}
				return -1;
			}

			void InsertOrUpdate(int32_t Idx)
			{
				const FTactixWorldState& S = Nodes[Idx].State;
				const uint64_t H    = S.Hash();
				const uint32_t Mask = Capacity - 1;
				for (uint32_t p = 0; p < Capacity; ++p)
				{
					const uint32_t Slot = static_cast<uint32_t>((H + p) & Mask);
					const int32_t  Cur  = Slots[Slot];
					if (Cur < 0 || Nodes[Cur].State == S)
					{
						Slots[Slot] = Idx;
						return;
					}
				}
			}
		};

		constexpr uint32_t NextPow2(uint32_t V)
		{
			if (V <= 1) return 1;
			V--; V |= V >> 1; V |= V >> 2; V |= V >> 4; V |= V >> 8; V |= V >> 16;
			return V + 1;
		}
	}

	FTactixGOAPPlan FTactixGOAPPlanner::Plan(
		const FTactixWorldState&   Start,
		const FTactixWorldState&   Goal,
		ITactixGOAPAction* const*  Actions,
		uint32_t                   ActionCount,
		const FTactixAgentContext& Ctx,
		FTactixArena&              Scratch,
		FTactixGOAPConfig          Config)
	{
		FTactixGOAPPlan Result{};

		if (Config.MaxNodes == 0 || Config.MaxPlanLength == 0) return Result;

		// Already satisfied -> empty plan, bValid=true, zero cost.
		if (Start.Satisfies(Goal)) { Result.bValid = true; return Result; }

		FNode*   Nodes = Scratch.EmplaceArrayUninitialised<FNode>  (Config.MaxNodes);
		int32_t* Heap  = Scratch.EmplaceArrayUninitialised<int32_t>(Config.MaxNodes);
		if (Nodes == nullptr || Heap == nullptr) return Result;

		const uint32_t ClosedCap = NextPow2(Config.MaxNodes * 2u);
		int32_t* Closed = Scratch.EmplaceArrayUninitialised<int32_t>(ClosedCap);
		if (Closed == nullptr) return Result;
		for (uint32_t i = 0; i < ClosedCap; ++i) Closed[i] = -1;

		FOpenSet   Open      { Heap,   0u, Config.MaxNodes, Nodes };
		FClosedSet ClosedSet { Closed, ClosedCap,           Nodes };

		uint32_t NodeCount = 0;
		Nodes[NodeCount] = { Start, -1, -1, 0.0f, static_cast<float>(Start.Distance(Goal)) };
		ClosedSet.InsertOrUpdate(static_cast<int32_t>(NodeCount));
		Open.Push(static_cast<int32_t>(NodeCount));
		++NodeCount;

		int32_t GoalIdx = -1;

		while (Open.Size > 0)
		{
			const int32_t CurIdx = Open.Pop();
			const FNode   Cur    = Nodes[CurIdx];

			if (Cur.State.Satisfies(Goal)) { GoalIdx = CurIdx; break; }

			// Stale entry: a better node for this state was found after we pushed.
			if (ClosedSet.Find(Cur.State) != CurIdx) continue;

			for (uint32_t a = 0; a < ActionCount; ++a)
			{
				ITactixGOAPAction* A = Actions[a];
				if (A == nullptr) continue;
				if (!A->IsApplicable(Ctx)) continue;

				if (!Cur.State.Satisfies(A->GetPreconditions(Ctx))) continue;

				const float Cost = A->GetCost(Ctx);
				if (Cost < 0.0f) continue;

				FTactixWorldState Next = Cur.State;
				Next.Apply(A->GetEffects(Ctx));

				const float NewG = Cur.G + Cost;

				const int32_t Existing = ClosedSet.Find(Next);
				if (Existing >= 0 && Nodes[Existing].G <= NewG) continue;

				if (NodeCount >= Config.MaxNodes) return Result;

				const float NewF = NewG + static_cast<float>(Next.Distance(Goal));
				Nodes[NodeCount] = { Next, CurIdx, static_cast<int32_t>(a), NewG, NewF };
				ClosedSet.InsertOrUpdate(static_cast<int32_t>(NodeCount));
				if (!Open.Push(static_cast<int32_t>(NodeCount))) return Result;
				++NodeCount;
			}
		}

		if (GoalIdx < 0) return Result;

		uint32_t Len = 0;
		for (int32_t i = GoalIdx; i >= 0 && Nodes[i].ActionIdx >= 0; i = Nodes[i].Parent) ++Len;
		if (Len == 0 || Len > Config.MaxPlanLength) return Result;

		ITactixGOAPAction** Steps = Scratch.EmplaceArrayUninitialised<ITactixGOAPAction*>(Len);
		if (Steps == nullptr) return Result;

		uint32_t Write = Len;
		for (int32_t i = GoalIdx; i >= 0 && Nodes[i].ActionIdx >= 0; i = Nodes[i].Parent)
		{
			Steps[--Write] = Actions[static_cast<uint32_t>(Nodes[i].ActionIdx)];
		}

		Result.Steps  = Steps;
		Result.Count  = Len;
		Result.Cost   = Nodes[GoalIdx].G;
		Result.bValid = true;
		return Result;
	}
}
