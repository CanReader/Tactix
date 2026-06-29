// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixGOAPPlanner.cpp
 * @brief A* implementation for @ref Tactix::FTactixGOAPPlanner.
 *
 * The pieces are a node pool, a binary min-heap as the open set, and a
 * linear-probing hash table as the closed set, all living in the caller's arena.
 * The contract is documented on the header; here we only flag the non-obvious
 * search mechanics (lazy deletion, the closed-set-as-best-known-G trick).
 */

#include "GOAP/FTactixGOAPPlanner.h"
#include "GOAP/ITactixGOAPAction.h"
#include "Foundation/TactixArena.h"

namespace Tactix
{
	namespace
	{
		/** @brief One A* search node: a state plus its back-link and scores. */
		struct FNode
		{
			FTactixWorldState State;     ///< World state at this node.
			int32_t           Parent;    ///< Index of the node we came from, or -1 at the start.
			int32_t           ActionIdx; ///< Action taken to reach here, or -1 at the start.
			float             G;         ///< Cost so far from the start.
			float             F;         ///< G + heuristic estimate to the goal.
		};

		/**
		 * @brief Binary min-heap over node indices, ordered by each node's F score.
		 *
		 * Stores indices, not nodes, so reordering is cheap and the nodes stay put
		 * in the pool (where parents are referenced by index).
		 */
		struct FOpenSet
		{
			int32_t*     Heap;     ///< Heap array of node indices.
			uint32_t     Size;     ///< Current element count.
			uint32_t     Capacity; ///< Allocated capacity.
			const FNode* Nodes;    ///< Node pool the indices refer to, for F lookups.

			/** @brief Pushes a node index, sifting up by F. Returns false if full. */
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

			/** @brief Pops the lowest-F node index, sifting down. Returns -1 if empty. */
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

		/**
		 * @brief Maps a world state to the best-known node index for it.
		 *
		 * Linear-probing table keyed on a state's hash; a slot of -1 is empty. It
		 * doubles as the "best G seen for this state" record: when a cheaper path to
		 * a state turns up, @ref InsertOrUpdate overwrites the slot, and the stale
		 * heap entry for the old node is dropped lazily in the main loop (a popped
		 * node whose state no longer maps back to it is skipped).
		 */
		struct FClosedSet
		{
			int32_t*     Slots;    ///< Slot array of node indices; -1 means empty.
			uint32_t     Capacity; ///< Power-of-two capacity for masked probing.
			const FNode* Nodes;    ///< Node pool the indices refer to.

			/** @brief Returns the node index stored for state @p S, or -1 if absent. */
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

			/** @brief Records @p Idx as the node for its state, replacing any prior one. */
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

		/** @brief Smallest power of two >= @p V (so the closed set can mask-probe). */
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

		// Reconstruct by walking parent links from the goal back to the start,
		// counting first to size the step array, then filling it back-to-front.
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
