// Copyright Sleak Software. All Rights Reserved.

/**
 * @file ATactixAIController.h
 * @brief Base AIController for Tactix-aware pawns.
 *
 * Two jobs. First, it owns the per-agent scratch arena that the GOAP and HTN
 * planners allocate from, so planning never touches the global heap. Second, it
 * holds the currently-executing plan (and a cursor into it) so the Behavior Tree
 * can step through a plan across frames without re-planning every tick.
 *
 * It's abstract on purpose: subclass it in project code to supply your GOAP
 * action library or HTN root task, then set the subclass as the pawn's AI
 * Controller Class.
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Foundation/TactixArena.h"
#include "GOAP/FTactixGOAPPlanner.h"
#include "HTN/FTactixHTNPlanner.h"
#include "ATactixAIController.generated.h"

class UTactixAgentComponent;

UCLASS(Abstract, Blueprintable)
class TACTIXUE_API ATactixAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATactixAIController(const FObjectInitializer& ObjectInitializer);

	/**
	 * @brief Mints the agent handle and assigns it to the pawn's component.
	 * @param InPawn The newly possessed pawn.
	 */
	virtual void OnPossess  (APawn* InPawn) override;
	/** @brief Clears plan state when the pawn is released. */
	virtual void OnUnPossess()              override;

	/**
	 * @brief The Tactix agent component on the controlled pawn.
	 * @return The component, or @c nullptr if the pawn doesn't have one.
	 */
	UFUNCTION(BlueprintPure, Category = "Tactix")
	UTactixAgentComponent* GetTactixAgent() const;

	/**
	 * @brief The scratch arena used for plan allocation.
	 * @return Reference to this controller's arena.
	 * @warning Call @ref Tactix::FTactixArena::Reset before each planning pass.
	 *          Plans returned by the planners point into this arena, so resetting
	 *          it invalidates the previous plan. Don't reset while a plan from it
	 *          is still being executed.
	 */
	Tactix::FTactixArena& GetScratchArena();

	Tactix::FTactixGOAPPlan ActiveGOAPPlan{};  ///< Current GOAP plan; written by the Run task, stepped by Execute.
	uint32                  GOAPPlanStep{0};    ///< Index of the next GOAP step to run.
	bool                    bGOAPPlanActive{false}; ///< Whether a GOAP plan is currently executing.

	Tactix::FTactixHTNPlan  ActiveHTNPlan{};   ///< Current HTN plan.
	uint32                  HTNPlanStep{0};     ///< Index of the next HTN step to run.
	bool                    bHTNPlanActive{false};  ///< Whether an HTN plan is currently executing.

private:
	/** @brief Size of the per-controller scratch arena. */
	static constexpr SIZE_T kArenaSizeBytes = 64u * 1024u;
	// TUniquePtr sidesteps the deleted copy ctor when UHT generates the vtable helper.
	TUniquePtr<Tactix::FTactixArena> ScratchArena; ///< Lazily-created planning arena.
};
