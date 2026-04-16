// Copyright Sleak Software. All Rights Reserved.
//
// FTactixDecisionRecorder — ring buffer of AI decisions for replay scrubbing
// and post-mortem debugging. BT tasks call Record() whenever they select or
// execute an action; the editor UI or log dump shows the full recent history.
//
// Usage (from UTactixBTTask_RunUtility or similar):
//   FTactixDecisionRecord R;
//   R.AgentHandle = Ctx.AgentHandle;
//   R.ActionName  = WinnerName;
//   R.Score       = WinnerScore;
//   R.Source      = ETactixDecisionSource::Utility;
//   R.HealthRatio = Ctx.HealthRatio;
//   ...
//   FTactixDecisionRecorder::Get().Record(R);

#pragma once

#include "CoreMinimal.h"
#include "Foundation/TactixHandle.h"
#include "Agent/ITactixAgent.h"

UENUM()
enum class ETactixDecisionSource : uint8
{
	Utility  UMETA(DisplayName = "Utility AI"),
	GOAPPlan UMETA(DisplayName = "GOAP Plan"),
	HTNPlan  UMETA(DisplayName = "HTN Plan"),
};

struct TACTIXDEBUG_API FTactixDecisionRecord
{
	double       WallTimeSeconds{0.0};
	Tactix::FTactixHandle<Tactix::ITactixAgent> AgentHandle{};
	FName        ActionName{};
	float        Score{0.0f};
	ETactixDecisionSource Source{ETactixDecisionSource::Utility};

	// Agent state snapshot at decision time.
	float HealthRatio{1.0f};
	float AmmoRatio{1.0f};
	bool  bInCover{false};
	bool  bHasThreat{false};
};

class TACTIXDEBUG_API FTactixDecisionRecorder
{
public:
	static constexpr int32 kRingSize = 512;

	// Append a decision record. Overwrites the oldest entry when full.
	void Record(const FTactixDecisionRecord& Entry);

	// Return the last MaxEntries decisions for the specified agent (newest first).
	TArray<FTactixDecisionRecord> GetHistory(
	    Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
	    int32 MaxEntries = 32) const;

	// All entries in chronological order (oldest first).
	TArray<FTactixDecisionRecord> GetAll() const;

	// Print the most recent MaxEntries to UE_LOG.
	void DumpToLog(int32 MaxEntries = 20) const;

	void Reset() { Ring.Reset(); Head = 0; Count = 0; }
	int32 Num() const { return Count; }

	// Module-lifetime singleton.
	static FTactixDecisionRecorder& Get();

private:
	TArray<FTactixDecisionRecord> Ring;
	int32 Head{0};
	int32 Count{0};
};
