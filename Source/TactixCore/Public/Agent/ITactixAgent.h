// Copyright Sleak Software. All Rights Reserved.
//
// ITactixAgent — the minimal, engine-agnostic contract every "thinking"
// entity implements to participate in Tactix systems.
//
// UE actors adopt this interface via the `UTactixAgentComponent` adapter in
// TactixUE, which is the ONLY bridge point between Unreal's actor model and
// the pure-C++ core. Everything below TactixUE thinks of agents through
// `FTactixHandle<ITactixAgent>` + this interface, never through `AActor*`.

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"

#include <cstdint>

namespace Tactix
{
	struct FTactixAgentContext; // defined in FTactixAgentContext.h

	// Role an agent currently plays inside its squad (if any). Extensible via
	// `UserDefined` + a secondary lookup if projects need more granularity.
	enum class ETactixSquadRole : uint8_t
	{
		None = 0,
		Leader,
		Flanker,
		Suppressor,
		Medic,
		Scout,
		Assault,
		UserDefined,
	};

	class TACTIXCORE_API ITactixAgent
	{
	public:
		virtual ~ITactixAgent() = default;

		// Return a fresh read-only snapshot of this agent's state, filled by
		// the implementer from whatever backing store (UE actor, test stub,
		// etc.) they use. The returned struct is passed by const-ref to every
		// evaluator in Tactix, so it MUST be cheap to build and self-contained.
		virtual FTactixAgentContext BuildContext() const = 0;

		// Stable identity for cross-frame references. The same handle must
		// compare equal across successive BuildContext calls for the life of
		// the agent.
		virtual FTactixHandle<ITactixAgent> GetHandle() const = 0;
	};
}
