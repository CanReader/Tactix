// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixCurve.h
 * @brief Response curves that turn a raw input into a normalised utility score.
 *
 * The Utility AI scores actions by feeding each consideration's raw input
 * (health ratio, distance to threat, ammo left, ...) through a response curve.
 * The curve's whole job is to map that input into [0, 1] with a shape the
 * designer chose. Five canonical shapes cover almost everything in practice:
 *
 * | Type        | Formula                                  | Notes                |
 * |-------------|------------------------------------------|----------------------|
 * | Linear      | `y = Slope * (x - Shift)`                | saturated            |
 * | Quadratic   | `y = Slope * (x - Shift)^Exp`            | sign-preserving      |
 * | Logistic    | `y = 1 / (1 + e^(-Slope * (x - Shift)))` | natural S-curve      |
 * | Exponential | `y = Slope * (e^(Exp * (x - Shift)) - 1)`| saturated            |
 * | Custom      | `y = CustomFn(x)`                        | saturated            |
 *
 * Every output is clamped to [0, 1]. That clamp is a hard invariant of the
 * library, not a convenience: @c FTactixUtilitySelector multiplies considerations
 * together, and a single out-of-range factor would blow up the product. See
 * @ref Tactix::FTactixCurve::Evaluate "FTactixCurve::Evaluate" for the exact per-type handling.
 */

#pragma once

#include "TactixApi.h"

#include <cstdint>

namespace Tactix
{
	/** @brief Selects which formula @ref FTactixCurve::Evaluate applies. */
	enum class ETactixCurveType : uint8_t
	{
		Linear = 0,   ///< Straight line, clamped to [0, 1].
		Quadratic,    ///< Power curve that preserves the sign of `(x - Shift)`.
		Logistic,     ///< Sigmoid; naturally lands in (0, 1).
		Exponential,  ///< Exponential rise, clamped to [0, 1].
		Custom,       ///< Arbitrary user function, result clamped to [0, 1].
	};

	/** @brief Signature of a custom curve: takes the raw input, returns a score. */
	using FTactixCurveFn = float (*)(float);

	/**
	 * @brief A plain-data response curve: a shape plus its tuning parameters.
	 *
	 * Trivially copyable, cheap to pass by value, and safe to bake into data
	 * assets. Build one with the @c Make* factories rather than filling the fields
	 * by hand so unused parameters (e.g. @c Exp for a linear curve) stay at their
	 * neutral values.
	 */
	struct FTactixCurve
	{
		ETactixCurveType Type{ETactixCurveType::Linear};  ///< Which formula to evaluate.
		float            Slope{1.0f};                     ///< Gain. For logistic, steepness of the S.
		float            Exp{2.0f};                       ///< Exponent for quadratic/exponential shapes.
		float            Shift{0.0f};                     ///< Input offset; for logistic, the midpoint.
		FTactixCurveFn   CustomFn{nullptr};               ///< Used only when @c Type is Custom.

		/**
		 * @brief Evaluates the curve at @p X.
		 * @param X Raw consideration input. There's no required range; the shape
		 *          and the final clamp decide what comes out.
		 * @return A score in [0, 1]. A Custom curve with a null @c CustomFn returns 0.
		 */
		TACTIXCORE_API float Evaluate(float X) const noexcept;

		/**
		 * @brief Linear curve `y = Slope * (x - Shift)`.
		 * @param InSlope Gain applied after the shift.
		 * @param InShift Input offset.
		 */
		static constexpr FTactixCurve MakeLinear(float InSlope = 1.0f, float InShift = 0.0f)
		{
			return FTactixCurve{ETactixCurveType::Linear, InSlope, 0.0f, InShift, nullptr};
		}

		/**
		 * @brief Power curve `y = Slope * sign(x-Shift) * |x-Shift|^Exp`.
		 * @param InSlope Gain applied to the powered term.
		 * @param InExp   Exponent; 2 gives the familiar ease-in parabola.
		 * @param InShift Input offset.
		 */
		static constexpr FTactixCurve MakeQuadratic(float InSlope = 1.0f, float InExp = 2.0f, float InShift = 0.0f)
		{
			return FTactixCurve{ETactixCurveType::Quadratic, InSlope, InExp, InShift, nullptr};
		}

		/**
		 * @brief Logistic curve `y = 1 / (1 + e^(-Slope*(x-Shift)))`.
		 * @param InSlope Steepness of the transition; larger is sharper.
		 * @param InShift Input value of the curve's midpoint (where `y = 0.5`).
		 */
		static constexpr FTactixCurve MakeLogistic(float InSlope = 10.0f, float InShift = 0.5f)
		{
			return FTactixCurve{ETactixCurveType::Logistic, InSlope, 0.0f, InShift, nullptr};
		}

		/**
		 * @brief Exponential curve `y = Slope * (e^(Exp*(x-Shift)) - 1)`.
		 * @param InSlope Gain on the exponential term.
		 * @param InExp   Rate of growth.
		 * @param InShift Input offset.
		 */
		static constexpr FTactixCurve MakeExponential(float InSlope = 1.0f, float InExp = 4.0f, float InShift = 0.0f)
		{
			return FTactixCurve{ETactixCurveType::Exponential, InSlope, InExp, InShift, nullptr};
		}

		/**
		 * @brief Curve backed by a caller-supplied function.
		 * @param InFn Function evaluated for each input. Its result is still clamped
		 *             to [0, 1], so it doesn't have to clamp itself.
		 */
		static constexpr FTactixCurve MakeCustom(FTactixCurveFn InFn)
		{
			return FTactixCurve{ETactixCurveType::Custom, 0.0f, 0.0f, 0.0f, InFn};
		}
	};
}
