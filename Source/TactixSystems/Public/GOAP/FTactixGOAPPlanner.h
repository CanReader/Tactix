// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "GOAP/FTactixWorldState.h"

#include <cstdint>

namespace Tactix
{
	struct FTactixAgentContext;
	class  FTactixArena;
	class  ITactixGOAPAction;

	struct FTactixGOAPPlan
	{
		// Points into the arena the planner was given. Do not outlive Scratch.Reset().
		ITactixGOAPAction** Steps{nullptr};
		uint32_t            Count{0};
		float               Cost{0.0f};
		bool                bValid{false};
	};

	struct FTactixGOAPConfig
	{
		uint32_t MaxNodes      = 512;
		uint32_t MaxPlanLength = 32;
	};

	class TACTIXSYSTEMS_API FTactixGOAPPlanner
	{
	public:
		// A* over the world-state graph. Allocates everything from Scratch:
		// node storage, binary heap, closed-set hash table, and the step array
		// of the returned plan. Fails (bValid=false) if MaxNodes is exhausted
		// or the arena runs out.
		static FTactixGOAPPlan Plan(const FTactixWorldState&   Start,
		                            const FTactixWorldState&   Goal,
		                            ITactixGOAPAction* const*  Actions,
		                            uint32_t                   ActionCount,
		                            const FTactixAgentContext& Ctx,
		                            FTactixArena&              Scratch,
		                            FTactixGOAPConfig          Config = {});
	};
}
