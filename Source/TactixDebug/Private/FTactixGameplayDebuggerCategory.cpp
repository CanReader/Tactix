// Copyright Sleak Software. All Rights Reserved.

#include "FTactixGameplayDebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Controller/ATactixAIController.h"
#include "Components/UTactixAgentComponent.h"
#include "FTactixDecisionRecorder.h"

FTactixGameplayDebuggerCategory::FTactixGameplayDebuggerCategory()
{
	// Collect at 10 Hz — agent state doesn't change faster than that.
	CollectDataInterval = 0.1f;
	bShowDataPackReplication = false;
}

TSharedRef<FGameplayDebuggerCategory> FTactixGameplayDebuggerCategory::MakeInstance()
{
	return MakeShareable(new FTactixGameplayDebuggerCategory());
}

void FTactixGameplayDebuggerCategory::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	const APawn* Pawn = Cast<APawn>(DebugActor);
	if (!Pawn)
	{
		AddTextLine(FString(TEXT("{red}Selected actor is not a pawn")));
		return;
	}

	const ATactixAIController* Controller =
	    Cast<ATactixAIController>(Pawn->GetController());
	if (!Controller)
	{
		AddTextLine(FString(TEXT("{red}Pawn has no ATactixAIController")));
		return;
	}

	const UTactixAgentComponent* Agent = Controller->GetTactixAgent();
	if (!Agent)
	{
		AddTextLine(FString(TEXT("{red}No UTactixAgentComponent found")));
		return;
	}

	// Vitals
	const float HP  = (Agent->MaxHealth  > 0.0f) ? Agent->CurrentHealth  / Agent->MaxHealth  : 0.0f;
	const float Amm = (Agent->MaxAmmo    > 0.0f) ? Agent->CurrentAmmo    / Agent->MaxAmmo    : 0.0f;
	const float Sta = (Agent->MaxStamina > 0.0f) ? Agent->CurrentStamina / Agent->MaxStamina : 0.0f;

	const auto PctColor = [](float R) -> const TCHAR*
	{
		return (R > 0.6f) ? TEXT("{green}") : (R > 0.3f) ? TEXT("{yellow}") : TEXT("{red}");
	};

	AddTextLine(FString::Printf(TEXT("{white}HP %s%.0f%%  {white}Ammo %s%.0f%%  {white}Sta %s%.0f%%"),
	    PctColor(HP),  HP  * 100.0f,
	    PctColor(Amm), Amm * 100.0f,
	    PctColor(Sta), Sta * 100.0f));

	// State flags
	FString State;
	if (Agent->bInCover)   State += TEXT("[Cover] ");
	if (Agent->bReloading) State += TEXT("[Reload] ");
	if (Agent->bHasThreat)
	{
		const float Dist = (float)(Pawn->GetActorLocation() - Agent->ThreatLocation).Size();
		State += FString::Printf(TEXT("[Threat %.0f uu]"), Dist);
	}
	AddTextLine(FString::Printf(TEXT("{white}State:  {yellow}%s"), State.IsEmpty() ? TEXT("–") : *State));

	// Active plan
	FString PlanStr;
	if (Controller->bGOAPPlanActive && Controller->ActiveGOAPPlan.bValid)
	{
		PlanStr = FString::Printf(TEXT("GOAP %u/%u"),
		    Controller->GOAPPlanStep + 1, Controller->ActiveGOAPPlan.Count);
	}
	else if (Controller->bHTNPlanActive && Controller->ActiveHTNPlan.bValid)
	{
		PlanStr = FString::Printf(TEXT("HTN %u/%u"),
		    Controller->HTNPlanStep + 1, Controller->ActiveHTNPlan.Count);
	}
	else
	{
		PlanStr = TEXT("idle");
	}
	AddTextLine(FString::Printf(TEXT("{white}Plan:   {cyan}%s"), *PlanStr));

	// Last decision from the recorder
	const auto History = FTactixDecisionRecorder::Get().GetHistory(Agent->GetHandle(), 1);
	if (!History.IsEmpty())
	{
		const FTactixDecisionRecord& Last = History[0];
		AddTextLine(FString::Printf(TEXT("{white}Action: {orange}%s  {white}score={orange}%.3f"),
		    *Last.ActionName.ToString(), Last.Score));
	}
	else
	{
		AddTextLine(FString(TEXT("{white}Action: {grey}–")));
	}
}

void FTactixGameplayDebuggerCategory::DrawData(APlayerController* OwnerPC,
                                                FGameplayDebuggerCanvasContext& CanvasContext)
{
	// AddTextLine entries are drawn automatically by the base class.
	// DrawData is called for custom canvas drawing beyond text.
}

#endif // WITH_GAMEPLAY_DEBUGGER
