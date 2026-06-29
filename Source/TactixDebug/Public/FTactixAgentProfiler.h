// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixAgentProfiler.h
 * @brief Per-agent timing recorder for spotting which agents and which phases
 *        cost the most.
 *
 * Samples are tagged by agent and by a context name ("GOAPPlan", "HTNPlan",
 * "UtilityEval", ...) and kept in a fixed ring, so old data ages out on its own.
 * Query worst-case and average microseconds per agent/context, or dump a top-N
 * report. The easy way to feed it is the RAII @ref FTactixTimerScope "FTactixTimerScope":
 *
 * @code
 * {
 *     FTactixTimerScope Timer(AgentHandle, "GOAPPlan");
 *     FTactixGOAPPlanner::Plan(...);
 * } // elapsed recorded when Timer leaves scope
 * @endcode
 *
 * Or record by hand:
 * @code
 * const uint64 Start = FTactixAgentProfiler::StartCycles();
 * DoWork();
 * FTactixAgentProfiler::Get().RecordSample(Handle, "Work", Start);
 * @endcode
 */

#pragma once

#include "CoreMinimal.h"
#include "Foundation/TactixHandle.h"
#include "Agent/ITactixAgent.h"

/** @brief One timing measurement: who, what, how long, and when. */
struct TACTIXDEBUG_API FTactixTimingSample
{
	Tactix::FTactixHandle<Tactix::ITactixAgent> AgentHandle{}; ///< Agent the work belonged to.
	FName    Context{};                  ///< Phase label, e.g. "GOAPPlan".
	double   ElapsedMicroseconds{0.0};   ///< Measured duration in microseconds.
	double   WallTimeSeconds{0.0};       ///< Game time the sample was recorded.
};

/** @brief Ring buffer of timing samples with simple aggregate queries. */
class TACTIXDEBUG_API FTactixAgentProfiler
{
public:
	/** @brief Number of samples retained before the oldest is overwritten. */
	static constexpr int32 kRingSize = 512;

	/**
	 * @brief Records a sample that started at @p StartCycles and ends now.
	 * @param Agent       Agent the work belonged to.
	 * @param Context     Phase label.
	 * @param StartCycles Cycle counter captured by @ref StartCycles before the work.
	 */
	void RecordSample(Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
	                  FName Context, uint64 StartCycles);

	/** @brief Current platform cycle counter; pair with @ref RecordSample. */
	static uint64 StartCycles();

	/**
	 * @brief Worst (largest) recorded duration for an agent.
	 * @param Agent   Agent to query.
	 * @param Context Phase to filter on, or @c NAME_None for all phases.
	 * @return Worst-case microseconds, or 0 if there are no matching samples.
	 */
	double GetWorstCase(Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
	                    FName Context = NAME_None) const;

	/**
	 * @brief Mean recorded duration for an agent.
	 * @param Agent   Agent to query.
	 * @param Context Phase to filter on, or @c NAME_None for all phases.
	 * @return Average microseconds, or 0 if there are no matching samples.
	 */
	double GetAverage  (Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
	                    FName Context = NAME_None) const;

	/**
	 * @brief Logs a worst-case summary of the costliest agents.
	 * @param TopN How many agents to list.
	 */
	void Report(int32 TopN = 10) const;

	/** @brief Clears all recorded samples. */
	void Reset() { Ring.Reset(); Head = 0; Count = 0; }

	/** @brief Raw access to the sample ring. */
	const TArray<FTactixTimingSample>& GetSamples() const { return Ring; }

	/**
	 * @brief The module-lifetime singleton.
	 * @return The shared profiler. Valid between the TactixDebug module's startup
	 *         and shutdown.
	 */
	static FTactixAgentProfiler& Get();

private:
	TArray<FTactixTimingSample> Ring;  ///< Sample storage, used as a ring of @ref kRingSize.
	int32 Head{0};                     ///< Write cursor into @c Ring.
	int32 Count{0};                    ///< Live sample count, capped at @ref kRingSize.
};

/**
 * @brief RAII timer that records its lifetime to the profiler on destruction.
 *
 * Construct one at the top of the scope you want to measure; it captures the
 * start cycle count and, when it goes out of scope, records the elapsed time
 * against the given agent and context.
 */
struct TACTIXDEBUG_API FTactixTimerScope
{
	/**
	 * @brief Starts timing.
	 * @param InAgent   Agent to attribute the time to.
	 * @param InContext Phase label for the sample.
	 */
	FTactixTimerScope(Tactix::FTactixHandle<Tactix::ITactixAgent> InAgent, FName InContext);
	/** @brief Records the elapsed time. */
	~FTactixTimerScope();

	Tactix::FTactixHandle<Tactix::ITactixAgent> Agent; ///< Agent being timed.
	FName   Context;     ///< Phase label.
	uint64  StartCycles; ///< Cycle count captured at construction.
};
