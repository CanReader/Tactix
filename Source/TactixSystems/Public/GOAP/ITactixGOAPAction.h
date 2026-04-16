// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "GOAP/FTactixWorldState.h"

namespace Tactix
{
	struct FTactixAgentContext;

	class TACTIXSYSTEMS_API ITactixGOAPAction
	{
	public:
		virtual ~ITactixGOAPAction() = default;

		virtual FTactixWorldState GetPreconditions(const FTactixAgentContext& Ctx) const = 0;
		virtual FTactixWorldState GetEffects      (const FTactixAgentContext& Ctx) const = 0;

		// Must be >= 0. Negative costs break the A* admissibility guarantee.
		virtual float GetCost(const FTactixAgentContext& Ctx) const = 0;

		// Extra gate on top of preconditions: agent-level feasibility (ammo,
		// animation lock, cooldown, etc.) that the world-state bits don't capture.
		virtual bool IsApplicable(const FTactixAgentContext& Ctx) const = 0;

		virtual void Execute(FTactixAgentContext& Ctx) = 0;
	};
}
