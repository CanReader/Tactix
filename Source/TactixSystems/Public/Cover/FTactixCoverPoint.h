// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Foundation/TactixMath.h"
#include "Agent/ITactixAgent.h"

#include <cstdint>

namespace Tactix
{
	enum class ETactixCoverHeight : uint8_t
	{
		Low,
		High,
	};

	// Position: where the agent stands/crouches.
	// Normal: points away from the wall, into the open space behind the agent.
	// A threat is "blocked" when it lies roughly in -Normal relative to Position.
	struct FTactixCoverPoint
	{
		FTactixVec3                 Position{};
		FTactixVec3                 Normal{1.0f, 0.0f, 0.0f};
		ETactixCoverHeight          Height{ETactixCoverHeight::Low};
		FTactixHandle<ITactixAgent> ClaimedBy{};

		bool IsClaimed() const { return ClaimedBy.IsValid(); }
	};
}
