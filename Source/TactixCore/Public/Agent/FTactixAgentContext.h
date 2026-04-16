// Copyright Sleak Software. All Rights Reserved.
//
// FTactixAgentContext — read-only, side-effect-free snapshot of an agent's
// current state. Every evaluator (Utility consideration, GOAP heuristic, HTN
// condition, cover scorer, etc.) takes this struct by `const &` and is
// FORBIDDEN from mutating the agent during scoring. Separating "read" from
// "act" is what lets us run evaluators in any order, memoise scores, or
// execute them off the game thread in the future.

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Foundation/TactixMath.h"
#include "Agent/ITactixAgent.h"

#include <cstdint>

namespace Tactix
{
	// Concrete blackboards are templated on capacity but derive from a
	// capacity-independent abstract base (`FTactixBlackboardRef`) so we can
	// point at them from contexts without forwarding a template parameter.
	class FTactixBlackboardRef;

	struct FTactixAgentContext
	{
		// ---- Identity -------------------------------------------------------
		FTactixHandle<ITactixAgent> AgentHandle{};

		// ---- Spatial state --------------------------------------------------
		FTactixVec3 Position{};
		FTactixVec3 Velocity{};
		FTactixVec3 Forward{0.0f, 1.0f, 0.0f};

		// ---- Normalised vitals (use Saturate on input to guarantee range) ---
		float HealthRatio{1.0f};
		float AmmoRatio{1.0f};
		float StaminaRatio{1.0f};

		// ---- Threat awareness ----------------------------------------------
		FTactixHandle<ITactixAgent> PrimaryThreat{};
		FTactixVec3                 ThreatLocation{};
		float                       ThreatDistance{-1.0f}; // negative = no threat

		// ---- Social / tactical ---------------------------------------------
		ETactixSquadRole SquadRole{ETactixSquadRole::None};
		bool             bInCover{false};
		bool             bReloading{false};

		// ---- Shared working memory (optional) -------------------------------
		FTactixBlackboardRef* Blackboard{nullptr};

		// ---- Frame context --------------------------------------------------
		double TimeSeconds{0.0};

		// ---- Convenience queries --------------------------------------------
		TACTIX_NODISCARD bool HasThreat() const { return ThreatDistance >= 0.0f && PrimaryThreat.IsValid(); }
	};
}
