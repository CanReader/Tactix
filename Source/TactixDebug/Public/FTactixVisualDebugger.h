// Copyright Sleak Software. All Rights Reserved.
//
// FTactixVisualDebugger — static helpers for rendering per-agent overlays at
// runtime (editor and development builds). All methods are no-ops in shipping.
//
// Typical usage: call DrawAgent from ATactixAIController's Tick, guarded by
// a console variable so level designers can toggle it without recompiling.

#pragma once

#include "CoreMinimal.h"
#include "GOAP/FTactixGOAPPlanner.h"
#include "HTN/FTactixHTNPlanner.h"
#include "Agent/FTactixAgentContext.h"

class ATactixAIController;
class UTactixUtilityAsset;

namespace Tactix { struct FTactixAgentContext; }

// Settings block passed to DrawAgent — lets callers pick which overlays to render.
struct TACTIXDEBUG_API FTactixDebugSettings
{
	bool  bShowVitalBars   = true;   // health/ammo/stamina progress bars
	bool  bShowStateLine   = true;   // in-cover + reloading flags
	bool  bShowThreatLine  = true;   // line from agent to threat position
	bool  bShowPlanSteps   = true;   // active GOAP or HTN step name
	bool  bShowUtilityBars = false;  // last utility score per action (needs scores array)
	float ZOffset          = 120.0f; // UU above agent origin where text starts
	float BarWidth         = 100.0f; // pixel width of on-screen bars (approximate)
};

class TACTIXDEBUG_API FTactixVisualDebugger
{
public:
	// Draw all overlays for one AI-controlled pawn. Safe to call every tick —
	// draw primitives have duration 0 (single frame).
	static void DrawAgent(const UWorld* World,
	                      const ATactixAIController* Controller,
	                      const FTactixDebugSettings& Settings = {});

	// Draw the active GOAP plan's step names above AgentPos.
	static void DrawGOAPPlan(const UWorld* World,
	                         const FVector& AgentPos,
	                         const Tactix::FTactixGOAPPlan& Plan,
	                         uint32 CurrentStep,
	                         float ZOffset = 120.0f);

	// Draw the active HTN plan's step count / current index above AgentPos.
	static void DrawHTNPlan(const UWorld* World,
	                        const FVector& AgentPos,
	                        const Tactix::FTactixHTNPlan& Plan,
	                        uint32 CurrentStep,
	                        float ZOffset = 120.0f);

	// Draw a mini horizontal bar chart showing (name, score) pairs.
	// Scores are expected in [0, 1].
	static void DrawUtilityScores(const UWorld* World,
	                              const FVector& AgentPos,
	                              const TArray<TPair<FName, float>>& ActionScores,
	                              float ZOffset = 200.0f);

private:
	// Colour-codes a [0,1] ratio: red at low, yellow at mid, green at high.
	static FColor RatioColor(float Ratio);
};
