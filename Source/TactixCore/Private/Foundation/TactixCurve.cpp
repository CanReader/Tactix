// Copyright Sleak Software. All Rights Reserved.

#include "Foundation/TactixCurve.h"
#include "Foundation/TactixMath.h"

#include <cmath>

namespace Tactix
{
	/**
	 * @brief Dispatches on @c Type and clamps the result into [0, 1].
	 *
	 * @details The clamp via @ref Saturate is applied to every branch, including
	 * Logistic which already lives in (0, 1), because extreme @c Slope values can
	 * push a float a hair past 1.0 through rounding. Quadratic needs special care:
	 * `std::pow` of a negative base with a fractional exponent is NaN, so we power
	 * the magnitude and copy the sign back, which keeps the curve sensible for
	 * inputs below @c Shift.
	 */
	float FTactixCurve::Evaluate(float X) const noexcept
	{
		switch (Type)
		{
			case ETactixCurveType::Linear:
			{
				const float Y = Slope * (X - Shift);
				return Saturate(Y);
			}

			case ETactixCurveType::Quadratic:
			{
				// std::pow of a negative base with a non-integer exponent is NaN;
				// we intentionally preserve the sign by raising |arg|^Exp and copying the sign back.
				const float Arg    = X - Shift;
				const float Magnitude = std::pow(std::fabs(Arg), Exp);
				const float Signed    = Arg < 0.0f ? -Magnitude : Magnitude;
				return Saturate(Slope * Signed);
			}

			case ETactixCurveType::Logistic:
			{
				// Classic sigmoid; already in (0, 1) but still saturate in case
				// of FP drift at extremes of Slope.
				const float Z = -Slope * (X - Shift);
				const float Y = 1.0f / (1.0f + std::exp(Z));
				return Saturate(Y);
			}

			case ETactixCurveType::Exponential:
			{
				const float Y = Slope * (std::exp(Exp * (X - Shift)) - 1.0f);
				return Saturate(Y);
			}

			case ETactixCurveType::Custom:
			{
				if (CustomFn == nullptr) return 0.0f;
				return Saturate(CustomFn(X));
			}
		}
		return 0.0f;
	}
}
