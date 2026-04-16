// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "GOAP/FTactixWorldState.h"

#include <cstdint>

namespace Tactix
{
	struct FTactixAgentContext;
	class  FTactixArena;
	class  ITactixHTNTask;
	class  ITactixHTNPrimitive;

	struct FTactixHTNPlan
	{
		ITactixHTNPrimitive** Steps{nullptr};
		uint32_t              Count{0};
		bool                  bValid{false};
	};

	struct FTactixHTNConfig
	{
		uint32_t MaxPlanLength      = 32;
		uint32_t MaxRecursionDepth  = 32;
		uint32_t MaxMethodSubtasks  = 16;
	};

	class TACTIXSYSTEMS_API FTactixHTNPlanner
	{
	public:
		// Depth-first decomposition with method-order fallback. On method
		// failure the planner rolls back state+plan to the snapshot taken
		// before that method. Plan memory comes from Scratch.
		static FTactixHTNPlan Plan(ITactixHTNTask*            Root,
		                           const FTactixWorldState&   InitialState,
		                           const FTactixAgentContext& Ctx,
		                           FTactixArena&              Scratch,
		                           FTactixHTNConfig           Config = {});
	};
}
