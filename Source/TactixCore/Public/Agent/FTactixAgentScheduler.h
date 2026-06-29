// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixAgentScheduler.h
 * @brief Min-heap tick distributor that spreads agent work across frames under a
 *        per-frame CPU budget.
 *
 * This exists to answer "how do I tick 200 agents in a 10 ms frame?". Ticking
 * every agent every frame doesn't fit, so each agent instead registers how often
 * it wants to think (@ref Tactix::FTactixAgentScheduleEntry::TickIntervalMs "TickIntervalMs")
 * and roughly how long a think costs
 * (@ref Tactix::FTactixAgentScheduleEntry::BudgetUs "BudgetUs"). Once a frame the
 * game thread calls @ref Tactix::FTactixAgentScheduler::Collect "Collect", which drains the
 * agents that are due, in time order, and stops as soon as the frame's CPU budget
 * is used up. Agents that didn't fit stay at the head of the heap and get first
 * pick next frame, so nobody starves indefinitely.
 *
 * Implementation choices worth knowing:
 *  - The heap is hand-rolled over a heap-allocated array; @c std::vector and
 *    @c std::priority_queue are off-limits per the project rules.
 *  - Entries are stored by value, not by pointer. Each is small, so a few hundred
 *    agents stay within a couple of cache-friendly kilobytes.
 *  - @ref Tactix::FTactixAgentScheduler::ReportTickCost "ReportTickCost" is optional but recommended: feed
 *    back real timings and the next Collect uses them instead of the estimate.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Agent/ITactixAgent.h"

#include <cstddef>
#include <cstdint>

namespace Tactix
{
	/** @brief One agent's scheduling record: when it's due and what it costs. */
	struct FTactixAgentScheduleEntry
	{
		FTactixHandle<ITactixAgent> Agent{};               ///< Agent this entry schedules.
		double                      NextTickTime{0.0};     ///< Game time (seconds) the agent is next due.
		uint32_t                    TickIntervalMs{100};   ///< Desired period between ticks, in milliseconds.
		uint32_t                    BudgetUs{200};         ///< Estimated tick cost (µs); used before any measurement exists.
		uint32_t                    Priority{0};           ///< Tie-breaker when two agents are due at the same time; higher wins.
		uint32_t                    LastSpentUs{0};        ///< Most recent measured tick cost (µs), 0 if never reported.
	};

	/**
	 * @brief Owns the schedule and parcels out which agents may tick each frame.
	 *
	 * Single-threaded: the owning AI controller/subsystem drives it from the game
	 * thread. Not safe to call concurrently.
	 */
	class TACTIXCORE_API FTactixAgentScheduler
	{
	public:
		/**
		 * @brief Constructs the scheduler with a starting heap capacity.
		 * @param InitialCapacity Slots to pre-allocate. The heap grows on demand if
		 *        more agents register; 0 is bumped to a small default.
		 */
		explicit FTactixAgentScheduler(std::size_t InitialCapacity = 256);
		~FTactixAgentScheduler();

		FTactixAgentScheduler(const FTactixAgentScheduler&)            = delete;
		FTactixAgentScheduler& operator=(const FTactixAgentScheduler&) = delete;

		/**
		 * @brief Adds an agent, or updates it if already scheduled.
		 * @param Entry The full schedule record. If @c Entry.Agent is already in the
		 *        heap its record is overwritten in place and re-sifted.
		 * @return True on success; false if @c Entry.Agent is an invalid handle.
		 */
		bool Register(const FTactixAgentScheduleEntry& Entry);

		/**
		 * @brief Removes an agent from the schedule.
		 * @param Agent Agent to drop.
		 * @return True if it was present and removed; false if it wasn't scheduled.
		 */
		bool Unregister(FTactixHandle<ITactixAgent> Agent);

		/**
		 * @brief Collects the agents that should tick this frame.
		 *
		 * Pops agents in due order while their @c NextTickTime has elapsed, summing
		 * expected costs and stopping once the next agent would push the total past
		 * @p FrameBudgetUs. The first due agent is always allowed through even if it
		 * alone exceeds the budget, otherwise a single expensive agent could be
		 * starved forever. Each collected agent is immediately rescheduled at
		 * `NowSeconds + TickIntervalMs/1000`, so the caller can just loop over the
		 * results and invoke each agent's tick.
		 *
		 * @param NowSeconds    Current game time in seconds.
		 * @param FrameBudgetUs Soft CPU ceiling for this frame's AI, in microseconds.
		 * @param OutAgents     Caller-owned buffer that receives the due agents.
		 * @param OutCapacity   Size of @p OutAgents; collection also stops when full.
		 * @return Number of agents written to @p OutAgents.
		 */
		std::size_t Collect(double                       NowSeconds,
		                    uint32_t                     FrameBudgetUs,
		                    FTactixHandle<ITactixAgent>* OutAgents,
		                    std::size_t                  OutCapacity);

		/**
		 * @brief Records the measured cost of an agent's last tick.
		 * @param Agent   Agent that just ticked.
		 * @param SpentUs Measured wall time in microseconds.
		 * @note No-op if @p Agent isn't scheduled. The value feeds the budget math
		 *       in the next @ref Collect, so a consistently expensive agent naturally
		 *       gets squeezed out earlier.
		 */
		void ReportTickCost(FTactixHandle<ITactixAgent> Agent, uint32_t SpentUs);

		/** @brief Number of scheduled agents. */
		TACTIX_NODISCARD std::size_t Num()      const { return Count; }
		/** @brief Current heap capacity in entries. */
		TACTIX_NODISCARD std::size_t Capacity() const { return Cap; }
		/** @brief Whether @p Agent is currently scheduled. */
		TACTIX_NODISCARD bool        Contains(FTactixHandle<ITactixAgent> Agent) const;

	private:
		FTactixAgentScheduleEntry* Heap{nullptr};  ///< Backing array, organised as a binary min-heap.
		std::size_t                Count{0};        ///< Number of live entries.
		std::size_t                Cap{0};          ///< Allocated capacity of @c Heap.

		/** @brief Reallocates @c Heap to at least @p NewCapacity, copying entries over. */
		void       Grow(std::size_t NewCapacity);
		/** @brief Restores the heap property upward from @p Index after an insert/decrease. */
		void       SiftUp(std::size_t Index);
		/** @brief Restores the heap property downward from @p Index after a pop/increase. */
		void       SiftDown(std::size_t Index);
		/** @brief Linear search for @p Agent. Returns its index, or @c Cap if absent. */
		std::size_t FindIndex(FTactixHandle<ITactixAgent> Agent) const;

		/**
		 * @brief Heap ordering predicate: earlier @c NextTickTime first, higher
		 *        @c Priority breaking ties.
		 *
		 * Priority is compared with `>` so that "more important" sorts as "smaller"
		 * for the min-heap, putting it nearer the root.
		 */
		static TACTIX_FORCEINLINE bool Less(const FTactixAgentScheduleEntry& A,
		                                    const FTactixAgentScheduleEntry& B)
		{
			if (A.NextTickTime != B.NextTickTime) return A.NextTickTime < B.NextTickTime;
			return A.Priority > B.Priority;
		}
	};
}
