// Copyright Sleak Software. All Rights Reserved.

/**
 * @file ITactixAgent.h
 * @brief The engine-agnostic contract a "thinking" entity implements to take
 *        part in Tactix systems.
 *
 * This is intentionally tiny. An agent only has to do two things: hand out a
 * read-only snapshot of itself and report a stable identity. UE actors satisfy
 * the contract through @c UTactixAgentComponent in TactixUE, which is the single
 * bridge between Unreal's actor model and the pure-C++ core. Everything below
 * TactixUE deals in @c FTactixHandle<ITactixAgent> and this interface, never an
 * @c AActor*, so the systems stay testable without an engine.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"

#include <cstdint>

namespace Tactix
{
	struct FTactixAgentContext; // defined in FTactixAgentContext.h

	/**
	 * @brief The role an agent is playing within its squad, if it has one.
	 *
	 * @c UserDefined is the escape hatch: a project that needs finer roles can
	 * use it as a marker and disambiguate through a side channel (e.g. a
	 * blackboard value) rather than editing this enum.
	 */
	enum class ETactixSquadRole : uint8_t
	{
		None = 0,     ///< Not in a squad, or no role assigned.
		Leader,       ///< Drives squad-level decisions; formation anchor.
		Flanker,      ///< Moves wide to attack from the side.
		Suppressor,   ///< Lays down covering fire.
		Medic,        ///< Prioritises reviving/supporting allies.
		Scout,        ///< Forward recon, perception-heavy.
		Assault,      ///< Front-line pusher.
		UserDefined,  ///< Project-specific role resolved elsewhere.
	};

	/**
	 * @brief Interface every Tactix agent implements.
	 *
	 * Implementations own their backing state (a UE actor, a unit-test stub, a
	 * headless sim entity) and expose it through these two calls.
	 */
	class TACTIXCORE_API ITactixAgent
	{
	public:
		virtual ~ITactixAgent() = default;

		/**
		 * @brief Builds a fresh, self-contained snapshot of the agent's state.
		 *
		 * Every evaluator in Tactix (utility considerations, GOAP heuristics, HTN
		 * conditions, cover scorers) receives the result by const reference, so it
		 * has to be cheap to construct and must not alias anything that could
		 * change underneath the evaluator.
		 *
		 * @return A populated @ref FTactixAgentContext for the current frame.
		 */
		virtual FTactixAgentContext BuildContext() const = 0;

		/**
		 * @brief The agent's stable identity for cross-frame references.
		 * @return A handle that compares equal across successive calls for as long
		 *         as the agent lives. Other systems store this, never a raw pointer.
		 */
		virtual FTactixHandle<ITactixAgent> GetHandle() const = 0;
	};
}
