// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixCurve.h"
#include "Utility/ITactixConsideration.h"

namespace Tactix
{
	struct FTactixAgentContext;

	using FTactixInputSelector = float (*)(const FTactixAgentContext&);

	struct FTactixConsideration final : public ITactixConsideration
	{
		FTactixInputSelector Input{nullptr};
		FTactixCurve         Curve{};

		float Evaluate(const FTactixAgentContext& Ctx) const override
		{
			if (Input == nullptr) return 0.0f;
			return Curve.Evaluate(Input(Ctx));
		}
	};
}
