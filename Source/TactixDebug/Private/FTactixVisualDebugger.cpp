// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixVisualDebugger.cpp
 * @brief Implements the agent debug overlays with debug-draw primitives.
 *
 * Compiled out entirely when @c ENABLE_DRAW_DEBUG is off.
 * @ref FTactixVisualDebugger::RatioColor "RatioColor" drives the red/yellow/green
 * tint used by the vital bars.
 */

#include "FTactixVisualDebugger.h"
#include "DrawDebugHelpers.h"
#include "Controller/ATactixAIController.h"
#include "Components/UTactixAgentComponent.h"
#include "GameFramework/Pawn.h"

#if ENABLE_DRAW_DEBUG

// ---- Colour helpers --------------------------------------------------------

FColor FTactixVisualDebugger::RatioColor(float R)
{
	R = FMath::Clamp(R, 0.0f, 1.0f);
	if (R < 0.5f)
	{
		// Red → Yellow
		return FColor(255, static_cast<uint8>(R * 2.0f * 255.0f), 0);
	}
	else
	{
		// Yellow → Green
		return FColor(static_cast<uint8>((1.0f - R) * 2.0f * 255.0f), 255, 0);
	}
}

// ---- DrawAgent -------------------------------------------------------------

void FTactixVisualDebugger::DrawAgent(const UWorld* World,
                                      const ATactixAIController* Controller,
                                      const FTactixDebugSettings& Settings)
{
	if (!World || !Controller) return;

	const UTactixAgentComponent* Agent = Controller->GetTactixAgent();
	if (!Agent) return;

	const APawn* Pawn = Controller->GetPawn();
	if (!Pawn) return;

	const FVector Base = Pawn->GetActorLocation();
	FVector TextPos    = Base + FVector(0.0f, 0.0f, Settings.ZOffset);

	// -- Vital bars --
	if (Settings.bShowVitalBars)
	{
		const float HP  = (Agent->MaxHealth  > 0.0f) ? (Agent->CurrentHealth  / Agent->MaxHealth)  : 0.0f;
		const float Amm = (Agent->MaxAmmo    > 0.0f) ? (Agent->CurrentAmmo    / Agent->MaxAmmo)    : 0.0f;
		const float Sta = (Agent->MaxStamina > 0.0f) ? (Agent->CurrentStamina / Agent->MaxStamina) : 0.0f;

		DrawDebugString(World, TextPos,
		    FString::Printf(TEXT("HP %.0f%%  Ammo %.0f%%  Sta %.0f%%"),
		        HP * 100.0f, Amm * 100.0f, Sta * 100.0f),
		    nullptr, RatioColor(HP), 0.0f, true, 1.1f);
		TextPos.Z += 22.0f;
	}

	// -- State flags --
	if (Settings.bShowStateLine)
	{
		FString State;
		if (Agent->bInCover)   State += TEXT("[COVER] ");
		if (Agent->bReloading) State += TEXT("[RELOAD] ");
		if (Agent->bHasThreat)
		{
			const float Dist = (float)(Pawn->GetActorLocation() - Agent->ThreatLocation).Size();
			State += FString::Printf(TEXT("[THREAT %.0fuu]"), Dist);
		}
		if (!State.IsEmpty())
		{
			DrawDebugString(World, TextPos, State, nullptr, FColor::Yellow, 0.0f, true, 1.0f);
			TextPos.Z += 22.0f;
		}
	}

	// -- Threat line --
	if (Settings.bShowThreatLine && Agent->bHasThreat)
	{
		DrawDebugLine(World, Base, Agent->ThreatLocation, FColor::Red, false, 0.0f, 0, 1.5f);
	}

	// -- GOAP plan steps --
	if (Settings.bShowPlanSteps)
	{
		DrawGOAPPlan(World, Base, Controller->ActiveGOAPPlan,
		             Controller->GOAPPlanStep, Settings.ZOffset + 22.0f * 2.0f);
	}
}

// ---- DrawGOAPPlan ----------------------------------------------------------

void FTactixVisualDebugger::DrawGOAPPlan(const UWorld* World,
                                          const FVector& AgentPos,
                                          const Tactix::FTactixGOAPPlan& Plan,
                                          uint32 CurrentStep,
                                          float ZOffset)
{
	if (!World || !Plan.bValid || Plan.Count == 0) return;

	const FVector TextPos = AgentPos + FVector(0.0f, 0.0f, ZOffset);
	DrawDebugString(World, TextPos,
	    FString::Printf(TEXT("GOAP step %u/%u"), CurrentStep + 1, Plan.Count),
	    nullptr, FColor::Cyan, 0.0f, true, 1.0f);
}

// ---- DrawHTNPlan -----------------------------------------------------------

void FTactixVisualDebugger::DrawHTNPlan(const UWorld* World,
                                         const FVector& AgentPos,
                                         const Tactix::FTactixHTNPlan& Plan,
                                         uint32 CurrentStep,
                                         float ZOffset)
{
	if (!World || !Plan.bValid || Plan.Count == 0) return;

	const FVector TextPos = AgentPos + FVector(0.0f, 0.0f, ZOffset);
	DrawDebugString(World, TextPos,
	    FString::Printf(TEXT("HTN step %u/%u"), CurrentStep + 1, Plan.Count),
	    nullptr, FColor::Cyan, 0.0f, true, 1.0f);
}

// ---- DrawUtilityScores -----------------------------------------------------

void FTactixVisualDebugger::DrawUtilityScores(const UWorld* World,
                                               const FVector& AgentPos,
                                               const TArray<TPair<FName, float>>& ActionScores,
                                               float ZOffset)
{
	if (!World || ActionScores.IsEmpty()) return;

	FVector TextPos = AgentPos + FVector(0.0f, 0.0f, ZOffset);
	for (const TPair<FName, float>& Pair : ActionScores)
	{
		const FColor C = RatioColor(Pair.Value);
		DrawDebugString(World, TextPos,
		    FString::Printf(TEXT("%s %.2f"), *Pair.Key.ToString(), Pair.Value),
		    nullptr, C, 0.0f, true, 0.9f);
		TextPos.Z += 18.0f;
	}
}

#else // ENABLE_DRAW_DEBUG

void FTactixVisualDebugger::DrawAgent(const UWorld*, const ATactixAIController*, const FTactixDebugSettings&) {}
void FTactixVisualDebugger::DrawGOAPPlan(const UWorld*, const FVector&, const Tactix::FTactixGOAPPlan&, uint32, float) {}
void FTactixVisualDebugger::DrawHTNPlan(const UWorld*, const FVector&, const Tactix::FTactixHTNPlan&, uint32, float) {}
void FTactixVisualDebugger::DrawUtilityScores(const UWorld*, const FVector&, const TArray<TPair<FName,float>>&, float) {}
FColor FTactixVisualDebugger::RatioColor(float) { return FColor::White; }

#endif // ENABLE_DRAW_DEBUG
