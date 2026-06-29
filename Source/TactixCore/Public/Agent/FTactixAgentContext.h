// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixAgentContext.h
 * @brief Read-only, side-effect-free snapshot of one agent for a single decision.
 *
 * This is the input every Tactix evaluator reads and none of them are allowed to
 * write. Utility considerations, GOAP heuristics, HTN conditions and cover
 * scorers all take it by const reference. Keeping "read the world" cleanly
 * separate from "act on the world" is what makes it safe to run evaluators in any
 * order, cache their scores, or eventually push them off the game thread, none of
 * which would hold if scoring could mutate the agent.
 *
 * @note The producer (@ref Tactix::ITactixAgent::BuildContext "ITactixAgent::BuildContext") is responsible for
 *       filling the normalised fields in range. The struct itself doesn't clamp.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Foundation/TactixMath.h"
#include "Agent/ITactixAgent.h"

#include <cstdint>

namespace Tactix
{
	// Concrete blackboards are templated on capacity but derive from a
	// capacity-independent abstract base (FTactixBlackboardRef), which is what
	// lets a context hold one without dragging in the capacity template param.
	class FTactixBlackboardRef;

	/**
	 * @brief Everything an evaluator needs to know about an agent this frame.
	 *
	 * Plain copyable data, cheap to build and pass around. Vitals are normalised
	 * ratios; spatial fields are in world space.
	 */
	struct FTactixAgentContext
	{
		/** @brief Identity of the agent this snapshot describes. */
		FTactixHandle<ITactixAgent> AgentHandle{};

		FTactixVec3 Position{};                 ///< World position.
		FTactixVec3 Velocity{};                 ///< World velocity.
		FTactixVec3 Forward{0.0f, 1.0f, 0.0f};  ///< Unit facing direction.

		float HealthRatio{1.0f};   ///< Health in [0, 1]; 1 is full.
		float AmmoRatio{1.0f};     ///< Ammo in [0, 1]; 1 is full.
		float StaminaRatio{1.0f};  ///< Stamina in [0, 1]; 1 is full.

		FTactixHandle<ITactixAgent> PrimaryThreat{};   ///< Current main threat, if any.
		FTactixVec3                 ThreatLocation{};  ///< Last known threat position.
		float                       ThreatDistance{-1.0f}; ///< Distance to the threat; negative means "no threat".

		ETactixSquadRole SquadRole{ETactixSquadRole::None}; ///< Role within the squad.
		bool             bInCover{false};                   ///< Whether the agent currently holds cover.
		bool             bReloading{false};                 ///< Whether a reload is in progress.

		/** @brief Optional shared working memory. May be null. */
		FTactixBlackboardRef* Blackboard{nullptr};

		/** @brief Game time, in seconds, the snapshot was taken at. */
		double TimeSeconds{0.0};

		/**
		 * @brief Whether the agent has a usable threat right now.
		 * @return True when both @ref ThreatDistance is non-negative and
		 *         @ref PrimaryThreat is a valid handle.
		 */
		TACTIX_NODISCARD bool HasThreat() const { return ThreatDistance >= 0.0f && PrimaryThreat.IsValid(); }
	};
}
