// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixDecisionRecorder.cpp
 * @brief Implements the decision ring buffer: recording, history/all queries,
 *        and the log dump.
 *
 * @ref FTactixDecisionRecorder::Record "Record" stamps each entry with wall time. The query helpers do the ring
 * index arithmetic to read newest-first (history, dump) or oldest-first (getAll),
 * with the extra `+ kRingSize` terms keeping the modulo positive.
 */

#include "FTactixDecisionRecorder.h"
#include "HAL/PlatformTime.h"
#include "Logging/LogMacros.h"
#include "TactixCore.h"

// ---- Singleton -------------------------------------------------------------

static FTactixDecisionRecorder GDecisionRecorder;

FTactixDecisionRecorder& FTactixDecisionRecorder::Get()
{
	return GDecisionRecorder;
}

// ---- Recording -------------------------------------------------------------

void FTactixDecisionRecorder::Record(const FTactixDecisionRecord& Entry)
{
	FTactixDecisionRecord Stamped = Entry;
	Stamped.WallTimeSeconds = FPlatformTime::Seconds();

	if (Ring.Num() < kRingSize)
	{
		Ring.Add(Stamped);
	}
	else
	{
		Ring[Head] = Stamped;
	}
	Head  = (Head + 1) % kRingSize;
	Count = FMath::Min(Count + 1, kRingSize);
}

// ---- Query -----------------------------------------------------------------

TArray<FTactixDecisionRecord> FTactixDecisionRecorder::GetHistory(
    Tactix::FTactixHandle<Tactix::ITactixAgent> Agent, int32 MaxEntries) const
{
	TArray<FTactixDecisionRecord> Result;
	// Walk backwards from the most recent entry.
	const int32 Total = Ring.Num();
	for (int32 i = 0; i < Total && Result.Num() < MaxEntries; ++i)
	{
		// Most recent first: index = (Head - 1 - i + kRingSize) % kRingSize
		const int32 Idx = ((Head - 1 - i) % kRingSize + kRingSize) % kRingSize;
		if (!Ring.IsValidIndex(Idx)) continue;
		if (Ring[Idx].AgentHandle == Agent)
		{
			Result.Add(Ring[Idx]);
		}
	}
	return Result;
}

TArray<FTactixDecisionRecord> FTactixDecisionRecorder::GetAll() const
{
	TArray<FTactixDecisionRecord> Result;
	const int32 Total = Ring.Num();
	// Return in chronological order.
	for (int32 i = 0; i < Total; ++i)
	{
		const int32 Idx = (Head - Total + i + kRingSize * 2) % kRingSize;
		if (Ring.IsValidIndex(Idx))
		{
			Result.Add(Ring[Idx]);
		}
	}
	return Result;
}

// ---- Log dump --------------------------------------------------------------

void FTactixDecisionRecorder::DumpToLog(int32 MaxEntries) const
{
	UE_LOG(LogTactix, Log, TEXT("--- TactixDecisionRecorder (last %d) ---"), MaxEntries);
	const int32 Total = Ring.Num();
	int32 Printed     = 0;
	for (int32 i = 0; i < Total && Printed < MaxEntries; ++i)
	{
		const int32 Idx = ((Head - 1 - i) % kRingSize + kRingSize) % kRingSize;
		if (!Ring.IsValidIndex(Idx)) continue;

		const FTactixDecisionRecord& R = Ring[Idx];
		static const TCHAR* Sources[] = { TEXT("Utility"), TEXT("GOAP"), TEXT("HTN") };
		UE_LOG(LogTactix, Log,
		       TEXT("  T+%.2fs  Agent=0x%08X  [%s] %s  score=%.3f  hp=%.0f%%  ammo=%.0f%%  cover=%s"),
		       R.WallTimeSeconds,
		       R.AgentHandle.Packed,
		       Sources[static_cast<uint8>(R.Source)],
		       *R.ActionName.ToString(),
		       R.Score,
		       R.HealthRatio * 100.0f,
		       R.AmmoRatio   * 100.0f,
		       R.bInCover ? TEXT("yes") : TEXT("no"));
		++Printed;
	}
}
