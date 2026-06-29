// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixVisualDebugger.h
 * @brief Static helpers that draw per-agent debug overlays in the world.
 *
 * Pure debug draw, no widgets. Call @ref FTactixVisualDebugger::DrawAgent from an
 * AI controller's tick, ideally behind a console variable so designers can toggle
 * it live. All primitives are single-frame, so calling every tick is the intended
 * pattern. Nothing here is meant for a shipping build.
 */

#pragma once

#include "CoreMinimal.h"
#include "GOAP/FTactixGOAPPlanner.h"
#include "HTN/FTactixHTNPlanner.h"
#include "Agent/FTactixAgentContext.h"

class ATactixAIController;
class UTactixUtilityAsset;

namespace Tactix { struct FTactixAgentContext; }

/** @brief Which overlays @ref FTactixVisualDebugger::DrawAgent should render, and how. */
struct TACTIXDEBUG_API FTactixDebugSettings
{
	bool  bShowVitalBars   = true;   ///< Health/ammo/stamina bars.
	bool  bShowStateLine   = true;   ///< In-cover and reloading flags.
	bool  bShowThreatLine  = true;   ///< A line from the agent to its threat.
	bool  bShowPlanSteps   = true;   ///< The active GOAP/HTN step.
	bool  bShowUtilityBars = false;  ///< Per-action utility scores (needs a scores array supplied separately).
	float ZOffset          = 120.0f; ///< UU above the agent origin where text starts.
	float BarWidth         = 100.0f; ///< Approximate on-screen bar width in pixels.
};

/** @brief Namespace-like holder of agent debug-draw routines. */
class TACTIXDEBUG_API FTactixVisualDebugger
{
public:
	/**
	 * @brief Draws every enabled overlay for one AI-controlled pawn.
	 * @param World      World to draw into.
	 * @param Controller The pawn's controller; its agent and plans supply the data.
	 * @param Settings   Which overlays to show. Safe to call each tick.
	 */
	static void DrawAgent(const UWorld* World,
	                      const ATactixAIController* Controller,
	                      const FTactixDebugSettings& Settings = {});

	/**
	 * @brief Draws a GOAP plan's step names above a point, current step highlighted.
	 * @param World       World to draw into.
	 * @param AgentPos    World anchor for the text column.
	 * @param Plan        The plan to render.
	 * @param CurrentStep Index of the step currently executing.
	 * @param ZOffset     Height above @p AgentPos to start drawing.
	 */
	static void DrawGOAPPlan(const UWorld* World,
	                         const FVector& AgentPos,
	                         const Tactix::FTactixGOAPPlan& Plan,
	                         uint32 CurrentStep,
	                         float ZOffset = 120.0f);

	/**
	 * @brief Draws an HTN plan's progress (current index of total) above a point.
	 * @param World       World to draw into.
	 * @param AgentPos    World anchor for the text.
	 * @param Plan        The plan to render.
	 * @param CurrentStep Index of the step currently executing.
	 * @param ZOffset     Height above @p AgentPos to start drawing.
	 */
	static void DrawHTNPlan(const UWorld* World,
	                        const FVector& AgentPos,
	                        const Tactix::FTactixHTNPlan& Plan,
	                        uint32 CurrentStep,
	                        float ZOffset = 120.0f);

	/**
	 * @brief Draws a small horizontal bar chart of action scores.
	 * @param World        World to draw into.
	 * @param AgentPos     World anchor for the chart.
	 * @param ActionScores (name, score) pairs; scores expected in [0, 1].
	 * @param ZOffset      Height above @p AgentPos to start drawing.
	 */
	static void DrawUtilityScores(const UWorld* World,
	                              const FVector& AgentPos,
	                              const TArray<TPair<FName, float>>& ActionScores,
	                              float ZOffset = 200.0f);

private:
	/** @brief Maps a [0, 1] ratio to a red/yellow/green health-bar colour. */
	static FColor RatioColor(float Ratio);
};
