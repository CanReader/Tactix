// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"

namespace Tactix
{
	struct FTactixAgentContext;

	class TACTIXSYSTEMS_API ITactixConsideration
	{
	public:
		virtual ~ITactixConsideration() = default;

		// result must be in [0, 1]; the selector multiplies considerations.
		virtual float Evaluate(const FTactixAgentContext& Ctx) const = 0;
	};
}
