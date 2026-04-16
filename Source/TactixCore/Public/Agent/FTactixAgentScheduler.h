// Copyright Sleak Software. All Rights Reserved.
//
// FTactixAgentScheduler — priority-queue based tick distributor with per-agent
// microsecond budgets.
//
// The scheduler solves the "10 ms frame, 200 agents" problem: if every agent
// tick cost 100 µs naively we'd blow the frame budget on AI alone. Instead
// each agent registers a desired tick interval (e.g. 100 ms) and a CPU budget
// (e.g. 200 µs). Every frame, the game thread calls Collect() — it walks the
// min-heap, drains agents whose NextTickTime has elapsed, and stops once the
// frame CPU budget is exhausted. Starved agents are kept at the head of the
// heap so they run first next frame.
//
// Design notes:
//   • Storage is a hand-rolled min-heap over a heap-allocated array. The
//     STL's <queue>/<vector> are disallowed by the CLAUDE.md rules.
//   • The heap stores FULL entries (not pointers) — each entry is 32 bytes,
//     so 256 agents fit in 8 KiB which is cache-friendly.
//   • ReportTickCost is optional: callers use it to adapt future priorities
//     (an agent that keeps overshooting its budget gets deprioritised).

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Agent/ITactixAgent.h"

#include <cstddef>
#include <cstdint>

namespace Tactix
{
	struct FTactixAgentScheduleEntry
	{
		FTactixHandle<ITactixAgent> Agent{};
		double                      NextTickTime{0.0};   // seconds (game time)
		uint32_t                    TickIntervalMs{100}; // desired period in ms
		uint32_t                    BudgetUs{200};       // wall-time budget per tick
		uint32_t                    Priority{0};         // tie-breaker (higher wins)
		uint32_t                    LastSpentUs{0};      // measured tick cost (µs)
	};

	class TACTIXCORE_API FTactixAgentScheduler
	{
	public:
		explicit FTactixAgentScheduler(std::size_t InitialCapacity = 256);
		~FTactixAgentScheduler();

		FTactixAgentScheduler(const FTactixAgentScheduler&)            = delete;
		FTactixAgentScheduler& operator=(const FTactixAgentScheduler&) = delete;

		// Register or update an agent. If `Entry.Agent` already exists its
		// record is replaced in place and the heap is re-sifted.
		bool Register(const FTactixAgentScheduleEntry& Entry);

		// Remove an agent from the schedule. Returns false if not found.
		bool Unregister(FTactixHandle<ITactixAgent> Agent);

		// Pop all agents whose NextTickTime <= NowSeconds, stopping early when
		// the sum of LastSpentUs exceeds FrameBudgetUs. Newly-drained agents
		// are re-scheduled at NowSeconds + TickIntervalMs/1000 immediately so
		// the caller may simply iterate and call agent->Tick().
		//
		// Returns the number of agents written to OutAgents.
		std::size_t Collect(double                       NowSeconds,
		                    uint32_t                     FrameBudgetUs,
		                    FTactixHandle<ITactixAgent>* OutAgents,
		                    std::size_t                  OutCapacity);

		// Record the measured tick cost so the next Collect() pass can make
		// informed budget decisions.
		void ReportTickCost(FTactixHandle<ITactixAgent> Agent, uint32_t SpentUs);

		TACTIX_NODISCARD std::size_t Num()      const { return Count; }
		TACTIX_NODISCARD std::size_t Capacity() const { return Cap; }
		TACTIX_NODISCARD bool        Contains(FTactixHandle<ITactixAgent> Agent) const;

	private:
		FTactixAgentScheduleEntry* Heap{nullptr};
		std::size_t                Count{0};
		std::size_t                Cap{0};

		void       Grow(std::size_t NewCapacity);
		void       SiftUp(std::size_t Index);
		void       SiftDown(std::size_t Index);
		std::size_t FindIndex(FTactixHandle<ITactixAgent> Agent) const;

		static TACTIX_FORCEINLINE bool Less(const FTactixAgentScheduleEntry& A,
		                                    const FTactixAgentScheduleEntry& B)
		{
			if (A.NextTickTime != B.NextTickTime) return A.NextTickTime < B.NextTickTime;
			// Higher priority wins ties (we invert so "higher priority" = "smaller for min-heap").
			return A.Priority > B.Priority;
		}
	};
}
