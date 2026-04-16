// Copyright Sleak Software. All Rights Reserved.

#include "FTactixAgentProfiler.h"
#include "HAL/PlatformTime.h"
#include "Logging/LogMacros.h"
#include "TactixCore.h"

// ---- Singleton -------------------------------------------------------------

static FTactixAgentProfiler GAgentProfiler;

FTactixAgentProfiler& FTactixAgentProfiler::Get()
{
	return GAgentProfiler;
}

// ---- Timing ----------------------------------------------------------------

uint64 FTactixAgentProfiler::StartCycles()
{
	return FPlatformTime::Cycles64();
}

void FTactixAgentProfiler::RecordSample(Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
                                         FName Context, uint64 Start)
{
	const uint64 End      = FPlatformTime::Cycles64();
	const double ElapsedUs = static_cast<double>(End - Start)
	                       * FPlatformTime::GetSecondsPerCycle64() * 1.0e6;

	FTactixTimingSample Sample;
	Sample.AgentHandle        = Agent;
	Sample.Context            = Context;
	Sample.ElapsedMicroseconds = ElapsedUs;
	Sample.WallTimeSeconds    = FPlatformTime::Seconds();

	if (Ring.Num() < kRingSize)
	{
		Ring.Add(Sample);
	}
	else
	{
		Ring[Head] = Sample;
	}
	Head  = (Head + 1) % kRingSize;
	Count = FMath::Min(Count + 1, kRingSize);
}

// ---- Query -----------------------------------------------------------------

double FTactixAgentProfiler::GetWorstCase(Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
                                           FName Context) const
{
	double Worst = 0.0;
	for (const FTactixTimingSample& S : Ring)
	{
		if (S.AgentHandle != Agent) continue;
		if (!Context.IsNone() && S.Context != Context) continue;
		Worst = FMath::Max(Worst, S.ElapsedMicroseconds);
	}
	return Worst;
}

double FTactixAgentProfiler::GetAverage(Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
                                         FName Context) const
{
	double Sum = 0.0;
	int32  N   = 0;
	for (const FTactixTimingSample& S : Ring)
	{
		if (S.AgentHandle != Agent) continue;
		if (!Context.IsNone() && S.Context != Context) continue;
		Sum += S.ElapsedMicroseconds;
		++N;
	}
	return (N > 0) ? Sum / static_cast<double>(N) : 0.0;
}

void FTactixAgentProfiler::Report(int32 TopN) const
{
	// Build a map of AgentHandle → worst-case for a ranked log dump.
	TMap<uint32, double> WorstPerAgent;
	for (const FTactixTimingSample& S : Ring)
	{
		const uint32 Key = S.AgentHandle.Packed;
		double& W = WorstPerAgent.FindOrAdd(Key, 0.0);
		W = FMath::Max(W, S.ElapsedMicroseconds);
	}

	// Sort descending.
	WorstPerAgent.ValueSort([](double A, double B) { return A > B; });

	UE_LOG(LogTactix, Log, TEXT("--- TactixAgentProfiler Report (top %d) ---"), TopN);
	int32 Rank = 0;
	for (const auto& Pair : WorstPerAgent)
	{
		UE_LOG(LogTactix, Log, TEXT("  [%d] Handle=0x%08X  WorstCase=%.1f µs"),
		       ++Rank, Pair.Key, Pair.Value);
		if (Rank >= TopN) break;
	}
}

// ---- RAII scope ------------------------------------------------------------

FTactixTimerScope::FTactixTimerScope(Tactix::FTactixHandle<Tactix::ITactixAgent> InAgent,
                                     FName InContext)
	: Agent(InAgent)
	, Context(InContext)
	, StartCycles(FTactixAgentProfiler::StartCycles())
{
}

FTactixTimerScope::~FTactixTimerScope()
{
	FTactixAgentProfiler::Get().RecordSample(Agent, Context, StartCycles);
}
