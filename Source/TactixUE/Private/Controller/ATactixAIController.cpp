// Copyright Sleak Software. All Rights Reserved.

#include "Controller/ATactixAIController.h"
#include "Components/UTactixAgentComponent.h"
#include "GameFramework/Pawn.h"

ATactixAIController::ATactixAIController()
	: ScratchArena(MakeUnique<Tactix::FTactixArena>(kArenaSizeBytes))
{
}

Tactix::FTactixArena& ATactixAIController::GetScratchArena()
{
	return *ScratchArena;
}

void ATactixAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UTactixAgentComponent* Agent = GetTactixAgent())
	{
		// Use the lower 16 bits of the pawn's unique ID as the handle index.
		// Generation 1 means "first occupant of this slot" — never zero.
		const uint32 Idx = InPawn->GetUniqueID() & 0xFFFFu;
		Agent->SetHandle(Tactix::FTactixHandle<Tactix::ITactixAgent>{Idx, 1u});
	}
}

void ATactixAIController::OnUnPossess()
{
	// Invalidate plan state — pointers live in the arena which we're about to reset.
	ActiveGOAPPlan  = {};
	GOAPPlanStep    = 0;
	bGOAPPlanActive = false;

	ActiveHTNPlan   = {};
	HTNPlanStep     = 0;
	bHTNPlanActive  = false;

	ScratchArena->Reset();

	Super::OnUnPossess();
}

UTactixAgentComponent* ATactixAIController::GetTactixAgent() const
{
	const APawn* Pawn = GetPawn();
	return Pawn ? Pawn->FindComponentByClass<UTactixAgentComponent>() : nullptr;
}
