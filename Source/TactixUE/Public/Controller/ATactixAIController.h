// Copyright Sleak Software. All Rights Reserved.
//
// Base AIController for Tactix-aware pawns. Owns the per-agent planning
// scratch arena and stores the active GOAP and HTN plans so BT tasks can
// advance them without re-running the planner every tick.
//
// Subclass this in project code to wire in project-specific GOAP action
// libraries or HTN root tasks, then assign it as the AI Controller Class
// on the AICharacter.

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
	ATactixAIController();

	virtual void OnPossess  (APawn* InPawn) override;
	virtual void OnUnPossess()              override;

	// ---- Agent access -------------------------------------------------------

	// Returns the Tactix component on the controlled pawn, or nullptr.
	UFUNCTION(BlueprintPure, Category = "Tactix")
	UTactixAgentComponent* GetTactixAgent() const;

	// ---- Planning scratch memory -------------------------------------------

	// Returns the arena used for plan allocation. The caller MUST call
	// ScratchArena.Reset() before each planning pass to reclaim memory.
	Tactix::FTactixArena& GetScratchArena();

	// ---- Active plan state (written by UTactixBTTask_Run*, read by Execute) -

	Tactix::FTactixGOAPPlan ActiveGOAPPlan{};
	uint32                  GOAPPlanStep{0};
	bool                    bGOAPPlanActive{false};

	Tactix::FTactixHTNPlan  ActiveHTNPlan{};
	uint32                  HTNPlanStep{0};
	bool                    bHTNPlanActive{false};

private:
	static constexpr SIZE_T kArenaSizeBytes = 64u * 1024u;
	// TUniquePtr avoids the deleted-copy-ctor issue with UHT's vtable helper ctor.
	TUniquePtr<Tactix::FTactixArena> ScratchArena;
};
