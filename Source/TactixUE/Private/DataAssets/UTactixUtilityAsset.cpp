// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixUtilityAsset.cpp
 * @brief Implements utility scoring: input extraction, curve building, and the
 *        Lewis-compensated action score.
 *
 * @c ScoreAction inlines the same compensation as
 * @ref Tactix::FTactixUtilitySelector::ScoreConsiderations rather than calling it,
 * because the considerations here are config structs, not @c ITactixConsideration
 * objects, so there's nothing to point an array of interface pointers at.
 */

#include "DataAssets/UTactixUtilityAsset.h"
#include "Foundation/TactixMath.h"

using namespace Tactix;

float UTactixUtilityAsset::GetRawInput(const FTactixConsiderationConfig& Cfg,
                                       const FTactixAgentContext& Ctx)
{
	switch (Cfg.InputType)
	{
	case ETactixInputType::HealthRatio:
		return Ctx.HealthRatio;

	case ETactixInputType::AmmoRatio:
		return Ctx.AmmoRatio;

	case ETactixInputType::StaminaRatio:
		return Ctx.StaminaRatio;

	case ETactixInputType::ThreatDistance:
		// Negative ThreatDistance means no known threat → input is 0.
		if (Ctx.ThreatDistance < 0.0f) return 0.0f;
		// Map [0, Cap] → [1, 0]: close threat = high urgency.
		return 1.0f - Clamp(Ctx.ThreatDistance / FMath::Max(Cfg.ThreatDistanceCap, 1.0f), 0.0f, 1.0f);

	case ETactixInputType::Speed:
		// Normalise against 600 uu/s (roughly a jog in UE defaults).
		return Clamp(Ctx.Velocity.Length() / 600.0f, 0.0f, 1.0f);

	case ETactixInputType::InCover:
		return Ctx.bInCover ? 1.0f : 0.0f;

	default:
		return 0.0f;
	}
}

// ---- Curve construction ----------------------------------------------------

FTactixCurve UTactixUtilityAsset::MakeCurve(const FTactixConsiderationConfig& Cfg)
{
	switch (Cfg.CurveType)
	{
	case ETactixCurveTypeBP::Linear:
		return FTactixCurve::MakeLinear(Cfg.Slope, Cfg.Shift);

	case ETactixCurveTypeBP::Quadratic:
		return FTactixCurve::MakeQuadratic(Cfg.Slope, Cfg.Exponent, Cfg.Shift);

	case ETactixCurveTypeBP::Logistic:
		return FTactixCurve::MakeLogistic(Cfg.Slope, Cfg.Shift);

	case ETactixCurveTypeBP::Exponential:
		return FTactixCurve::MakeExponential(Cfg.Slope, Cfg.Exponent, Cfg.Shift);

	default:
		return FTactixCurve::MakeLinear();
	}
}

// ---- Scoring ---------------------------------------------------------------

float UTactixUtilityAsset::ScoreAction(int32 Index, const FTactixAgentContext& Ctx) const
{
	if (!Actions.IsValidIndex(Index)) return 0.0f;
	const FTactixActionConfig& Action = Actions[Index];
	if (Action.Considerations.IsEmpty()) return 0.0f;

	const int32 N         = Action.Considerations.Num();
	const float ModFactor = 1.0f - (1.0f / static_cast<float>(N));
	float Product = 1.0f;

	for (const FTactixConsiderationConfig& Cfg : Action.Considerations)
	{
		const float Raw = GetRawInput(Cfg, Ctx);
		if (Raw <= 0.0f) return 0.0f;

		// Apply the response curve, clamp to [0, 1].
		float X = Clamp(MakeCurve(Cfg).Evaluate(Raw), 0.0f, 1.0f);
		if (X <= 0.0f) return 0.0f;

		// Mike Lewis compensation so extra considerations don't kill the score.
		const float MakeUp = (1.0f - X) * ModFactor;
		X += MakeUp * X;

		Product *= X;
	}

	return Product;
}

int32 UTactixUtilityAsset::PickBestAction(const FTactixAgentContext& Ctx,
                                          TArray<float>* OutScores) const
{
	int32 BestIdx   = INDEX_NONE;
	float BestScore = 0.0f;

	for (int32 i = 0; i < Actions.Num(); ++i)
	{
		const float S = ScoreAction(i, Ctx);
		if (OutScores) OutScores->Add(S);
		if (S > BestScore)
		{
			BestScore = S;
			BestIdx   = i;
		}
	}

	return BestIdx;
}
