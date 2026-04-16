// Copyright Sleak Software. All Rights Reserved.
//
// FTactixAgentProfiler — per-agent microsecond timing ring buffer. Records
// named timing samples and can report worst-case and average elapsed times
// per agent and per context (e.g. "GOAPPlan", "HTNPlan", "UtilityEval").
//
// Usage:
//   {
//       FTactixTimerScope Timer(AgentHandle, "GOAPPlan");
//       FTactixGOAPPlanner::Plan(...);
//   } // automatically records elapsed on destruction
//
// Or manually:
//   auto Start = FTactixAgentProfiler::StartCycles();
//   DoWork();
//   FTactixAgentProfiler::Get().RecordSample(Handle, "Work", Start);

#pragma once

#include "CoreMinimal.h"
#include "Foundation/TactixHandle.h"
#include "Agent/ITactixAgent.h"

struct TACTIXDEBUG_API FTactixTimingSample
{
	Tactix::FTactixHandle<Tactix::ITactixAgent> AgentHandle{};
	FName    Context{};
	double   ElapsedMicroseconds{0.0};
	double   WallTimeSeconds{0.0};
};

class TACTIXDEBUG_API FTactixAgentProfiler
{
public:
	static constexpr int32 kRingSize = 512;

	// Record a sample ending now. StartCycles comes from StartCycles().
	void RecordSample(Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
	                  FName Context, uint64 StartCycles);

	// Returns the current platform cycle counter.
	static uint64 StartCycles();

	// Query: worst-case and average elapsed µs over all recorded samples for
	// the given agent and context (empty Name = all contexts).
	double GetWorstCase(Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
	                    FName Context = NAME_None) const;

	double GetAverage  (Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
	                    FName Context = NAME_None) const;

	// Log a formatted summary of the top N worst-case agents.
	void Report(int32 TopN = 10) const;

	void Reset() { Ring.Reset(); Head = 0; Count = 0; }

	const TArray<FTactixTimingSample>& GetSamples() const { return Ring; }

	// Module-lifetime singleton. Valid between TactixDebug's StartupModule
	// and ShutdownModule.
	static FTactixAgentProfiler& Get();

private:
	TArray<FTactixTimingSample> Ring;
	int32 Head{0};
	int32 Count{0};
};

// RAII scope timer. Records elapsed time when it leaves scope.
struct TACTIXDEBUG_API FTactixTimerScope
{
	FTactixTimerScope(Tactix::FTactixHandle<Tactix::ITactixAgent> InAgent, FName InContext);
	~FTactixTimerScope();

	Tactix::FTactixHandle<Tactix::ITactixAgent> Agent;
	FName   Context;
	uint64  StartCycles;
};
