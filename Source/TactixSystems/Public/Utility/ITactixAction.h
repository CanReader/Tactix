// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"

namespace Tactix
{
	struct FTactixAgentContext;

	class TACTIXSYSTEMS_API ITactixAction
	{
	public:
		virtual ~ITactixAction() = default;

		virtual float GetScore(const FTactixAgentContext& Ctx) const = 0;
		virtual bool  IsValid (const FTactixAgentContext& Ctx) const = 0;
		virtual void  Execute (FTactixAgentContext& Ctx)            = 0;
	};
}
