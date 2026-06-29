// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixDecisionRecorder.h
 * @brief Ring buffer of recent AI decisions for replay scrubbing and
 *        post-mortems ("why did it do that?").
 *
 * Whenever an agent selects or executes an action, the deciding code records a
 * snapshot: the action, its score, the source system, and a little agent state.
 * Later you can pull one agent's history, dump everything to the log, or feed an
 * editor timeline. Decisions are fed in from BT tasks like
 * @ref UTactixBTTask_RunUtility "UTactixBTTask_RunUtility":
 *
 * @code
 * FTactixDecisionRecord R;
 * R.AgentHandle = Ctx.AgentHandle;
 * R.ActionName  = WinnerName;
 * R.Score       = WinnerScore;
 * R.Source      = ETactixDecisionSource::Utility;
 * R.HealthRatio = Ctx.HealthRatio;
 * FTactixDecisionRecorder::Get().Record(R);
 * @endcode
 */

#pragma once

#include "CoreMinimal.h"
#include "Foundation/TactixHandle.h"
#include "Agent/ITactixAgent.h"

/** @brief Which subsystem produced a recorded decision. */
UENUM()
enum class ETactixDecisionSource : uint8
{
	Utility  UMETA(DisplayName = "Utility AI"), ///< From the utility selector.
	GOAPPlan UMETA(DisplayName = "GOAP Plan"),  ///< A GOAP plan step.
	HTNPlan  UMETA(DisplayName = "HTN Plan"),   ///< An HTN plan step.
};

/** @brief One decision plus a snapshot of the state that produced it. */
struct TACTIXDEBUG_API FTactixDecisionRecord
{
	double       WallTimeSeconds{0.0};   ///< Game time the decision was made.
	Tactix::FTactixHandle<Tactix::ITactixAgent> AgentHandle{}; ///< Deciding agent.
	FName        ActionName{};           ///< Chosen action.
	float        Score{0.0f};            ///< Its score, where applicable.
	ETactixDecisionSource Source{ETactixDecisionSource::Utility}; ///< Producing system.

	float HealthRatio{1.0f};  ///< Health at decision time.
	float AmmoRatio{1.0f};    ///< Ammo at decision time.
	bool  bInCover{false};    ///< Whether the agent was in cover.
	bool  bHasThreat{false};  ///< Whether the agent had a threat.
};

/** @brief Fixed-size ring of decision records with per-agent and bulk queries. */
class TACTIXDEBUG_API FTactixDecisionRecorder
{
public:
	/** @brief Records retained before the oldest is overwritten. */
	static constexpr int32 kRingSize = 512;

	/**
	 * @brief Appends a decision, overwriting the oldest when the ring is full.
	 * @param Entry The record to store.
	 */
	void Record(const FTactixDecisionRecord& Entry);

	/**
	 * @brief Returns one agent's recent decisions, newest first.
	 * @param Agent      Agent to filter on.
	 * @param MaxEntries Cap on how many to return.
	 */
	TArray<FTactixDecisionRecord> GetHistory(
	    Tactix::FTactixHandle<Tactix::ITactixAgent> Agent,
	    int32 MaxEntries = 32) const;

	/** @brief All retained records in chronological order (oldest first). */
	TArray<FTactixDecisionRecord> GetAll() const;

	/**
	 * @brief Logs the most recent decisions.
	 * @param MaxEntries How many to print.
	 */
	void DumpToLog(int32 MaxEntries = 20) const;

	/** @brief Clears all records. */
	void Reset() { Ring.Reset(); Head = 0; Count = 0; }
	/** @brief Number of records currently held. */
	int32 Num() const { return Count; }

	/**
	 * @brief The module-lifetime singleton.
	 * @return The shared recorder. Valid while the TactixDebug module is loaded.
	 */
	static FTactixDecisionRecorder& Get();

private:
	TArray<FTactixDecisionRecord> Ring; ///< Record storage, used as a ring of @ref kRingSize.
	int32 Head{0};                      ///< Write cursor.
	int32 Count{0};                     ///< Live record count, capped at @ref kRingSize.
};
