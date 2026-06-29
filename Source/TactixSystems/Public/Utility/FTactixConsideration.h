// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixConsideration.h
 * @brief The standard consideration: pull one number out of the context, run it
 *        through a response curve.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixCurve.h"
#include "Utility/ITactixConsideration.h"

namespace Tactix
{
	struct FTactixAgentContext;

	/** @brief Extracts the raw input value for a consideration from agent state. */
	using FTactixInputSelector = float (*)(const FTactixAgentContext&);

	/**
	 * @brief Concrete consideration = input selector + response curve.
	 *
	 * This covers the overwhelming majority of considerations without subclassing:
	 * point @c Input at a small function that reads one field (health, distance,
	 * ammo) and pick a @c Curve to shape it. Designers tune the curve; the selector
	 * stays generic.
	 */
	struct FTactixConsideration final : public ITactixConsideration
	{
		FTactixInputSelector Input{nullptr};  ///< Reads the raw value from the context. Null means "score 0".
		FTactixCurve         Curve{};         ///< Maps that raw value into [0, 1].

		/**
		 * @brief Reads the input and shapes it through the curve.
		 * @param Ctx Read-only agent state.
		 * @return The curve's output in [0, 1], or 0 if no @c Input is set.
		 */
		float Evaluate(const FTactixAgentContext& Ctx) const override
		{
			if (Input == nullptr) return 0.0f;
			return Curve.Evaluate(Input(Ctx));
		}
	};
}
