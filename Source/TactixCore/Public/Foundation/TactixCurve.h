// Copyright Sleak Software. All Rights Reserved.
//
// TactixCurve — response curves used by the Utility AI consideration system.
//
// A consideration transforms a raw input (e.g. current health ratio) into a
// normalised score in [0, 1]. Designers pick from five canonical shapes:
//
//   Linear       y = Slope * (x - Shift)                       , saturated
//   Quadratic    y = Slope * (x - Shift)^Exp                   , saturated
//   Logistic     y = 1 / (1 + e^(-Slope * (x - Shift)))        , naturally in (0,1)
//   Exponential  y = Slope * (e^(Exp * (x - Shift)) - 1)       , saturated
//   Custom       y = CustomFn(x)                               , saturated
//
// All outputs are clamped to [0, 1] — this is a hard library invariant so
// the utility selector can multiply scores without running out of range.

#pragma once

#include "TactixApi.h"

#include <cstdint>

namespace Tactix
{
	enum class ETactixCurveType : uint8_t
	{
		Linear = 0,
		Quadratic,
		Logistic,
		Exponential,
		Custom,
	};

	using FTactixCurveFn = float (*)(float);

	struct FTactixCurve
	{
		ETactixCurveType Type{ETactixCurveType::Linear};
		float            Slope{1.0f};
		float            Exp{2.0f};
		float            Shift{0.0f};
		FTactixCurveFn   CustomFn{nullptr};

		// Evaluate the curve at `X` and return a value in [0, 1].
		TACTIXCORE_API float Evaluate(float X) const noexcept;

		// ---- Factories (keep tuning data out of constructors) ---------------

		static constexpr FTactixCurve MakeLinear(float InSlope = 1.0f, float InShift = 0.0f)
		{
			return FTactixCurve{ETactixCurveType::Linear, InSlope, 0.0f, InShift, nullptr};
		}

		static constexpr FTactixCurve MakeQuadratic(float InSlope = 1.0f, float InExp = 2.0f, float InShift = 0.0f)
		{
			return FTactixCurve{ETactixCurveType::Quadratic, InSlope, InExp, InShift, nullptr};
		}

		static constexpr FTactixCurve MakeLogistic(float InSlope = 10.0f, float InShift = 0.5f)
		{
			return FTactixCurve{ETactixCurveType::Logistic, InSlope, 0.0f, InShift, nullptr};
		}

		static constexpr FTactixCurve MakeExponential(float InSlope = 1.0f, float InExp = 4.0f, float InShift = 0.0f)
		{
			return FTactixCurve{ETactixCurveType::Exponential, InSlope, InExp, InShift, nullptr};
		}

		static constexpr FTactixCurve MakeCustom(FTactixCurveFn InFn)
		{
			return FTactixCurve{ETactixCurveType::Custom, 0.0f, 0.0f, 0.0f, InFn};
		}
	};
}
