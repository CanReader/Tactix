// Copyright Sleak Software. All Rights Reserved.

#include <gtest/gtest.h>

#include "Utility/ITactixAction.h"
#include "Utility/FTactixConsideration.h"
#include "Utility/FTactixUtilitySelector.h"
#include "Agent/FTactixAgentContext.h"
#include "Foundation/TactixCurve.h"

using namespace Tactix;

namespace
{
	struct StubAction : public ITactixAction
	{
		float Score{0.0f};
		bool  bValid{true};
		int   Executed{0};

		StubAction() = default;
		StubAction(float S, bool V = true) : Score(S), bValid(V) {}

		float GetScore(const FTactixAgentContext&) const override { return Score; }
		bool  IsValid (const FTactixAgentContext&) const override { return bValid; }
		void  Execute (FTactixAgentContext&) override             { ++Executed; }
	};

	float HealthInput(const FTactixAgentContext& C) { return C.HealthRatio; }
	float AmmoInput  (const FTactixAgentContext& C) { return C.AmmoRatio; }
}

TEST(UtilitySelector, PicksHighestScoredAmongValid)
{
	FTactixAgentContext Ctx{};
	StubAction A(0.2f), B(0.8f), C(0.5f);
	ITactixAction* Actions[] = { &A, &B, &C };

	const FTactixUtilityResult R = FTactixUtilitySelector::Pick(Actions, 3, Ctx);
	EXPECT_TRUE(R.HasWinner());
	EXPECT_EQ(R.Index, 1u);
	EXPECT_FLOAT_EQ(R.Score, 0.8f);
	EXPECT_EQ(R.Action, &B);
}

TEST(UtilitySelector, SkipsInvalidActions)
{
	FTactixAgentContext Ctx{};
	StubAction A(0.9f, false);
	StubAction B(0.5f, true);
	ITactixAction* Actions[] = { &A, &B };

	const FTactixUtilityResult R = FTactixUtilitySelector::Pick(Actions, 2, Ctx);
	EXPECT_TRUE(R.HasWinner());
	EXPECT_EQ(R.Index, 1u);
}

TEST(UtilitySelector, NoWinnerWhenAllInvalid)
{
	FTactixAgentContext Ctx{};
	StubAction A(0.9f, false);
	StubAction B(0.5f, false);
	ITactixAction* Actions[] = { &A, &B };

	const FTactixUtilityResult R = FTactixUtilitySelector::Pick(Actions, 2, Ctx);
	EXPECT_FALSE(R.HasWinner());
	EXPECT_EQ(R.Index, FTactixUtilityResult::kNoWinner);
	EXPECT_EQ(R.Action, nullptr);
}

TEST(UtilitySelector, NoWinnerWhenAllScoresZero)
{
	FTactixAgentContext Ctx{};
	StubAction A(0.0f), B(0.0f);
	ITactixAction* Actions[] = { &A, &B };

	const FTactixUtilityResult R = FTactixUtilitySelector::Pick(Actions, 2, Ctx);
	EXPECT_FALSE(R.HasWinner());
}

TEST(UtilitySelector, TiesGoToLowerIndex)
{
	FTactixAgentContext Ctx{};
	StubAction A(0.5f), B(0.5f);
	ITactixAction* Actions[] = { &A, &B };

	const FTactixUtilityResult R = FTactixUtilitySelector::Pick(Actions, 2, Ctx);
	EXPECT_TRUE(R.HasWinner());
	EXPECT_EQ(R.Index, 0u);
}

TEST(UtilitySelector, HandlesNullActionPointersSafely)
{
	FTactixAgentContext Ctx{};
	StubAction B(0.5f);
	ITactixAction* Actions[] = { nullptr, &B, nullptr };

	const FTactixUtilityResult R = FTactixUtilitySelector::Pick(Actions, 3, Ctx);
	EXPECT_TRUE(R.HasWinner());
	EXPECT_EQ(R.Index, 1u);
}

TEST(UtilityConsideration, MultipliesWithLewisCompensation)
{
	FTactixAgentContext Ctx{};
	Ctx.HealthRatio = 0.5f;
	Ctx.AmmoRatio   = 0.5f;

	const FTactixCurve Linear = FTactixCurve::MakeLinear();
	FTactixConsideration Health; Health.Input = HealthInput; Health.Curve = Linear;
	FTactixConsideration Ammo;  Ammo.Input  = AmmoInput;   Ammo.Curve  = Linear;

	const ITactixConsideration* Cons[] = { &Health, &Ammo };
	const float S = FTactixUtilitySelector::ScoreConsiderations(Cons, 2, Ctx);

	// Raw 0.5, N=2, ModFactor=0.5 -> X'=0.625. Product = 0.625 * 0.625.
	EXPECT_NEAR(S, 0.390625f, 1e-5f);
}

TEST(UtilityConsideration, ZeroInputShortCircuitsProduct)
{
	FTactixAgentContext Ctx{};
	Ctx.HealthRatio = 0.0f;
	Ctx.AmmoRatio   = 1.0f;

	const FTactixCurve Linear = FTactixCurve::MakeLinear();
	FTactixConsideration Health; Health.Input = HealthInput; Health.Curve = Linear;
	FTactixConsideration Ammo;  Ammo.Input  = AmmoInput;   Ammo.Curve  = Linear;

	const ITactixConsideration* Cons[] = { &Health, &Ammo };
	EXPECT_FLOAT_EQ(FTactixUtilitySelector::ScoreConsiderations(Cons, 2, Ctx), 0.0f);
}

TEST(UtilityConsideration, EmptyListReturnsZero)
{
	FTactixAgentContext Ctx{};
	EXPECT_FLOAT_EQ(FTactixUtilitySelector::ScoreConsiderations(nullptr, 0, Ctx), 0.0f);
}

TEST(UtilityConsideration, SingleConsiderationNoCompensation)
{
	FTactixAgentContext Ctx{};
	Ctx.HealthRatio = 0.7f;

	const FTactixCurve Linear = FTactixCurve::MakeLinear();
	FTactixConsideration Health; Health.Input = HealthInput; Health.Curve = Linear;
	const ITactixConsideration* Cons[] = { &Health };

	const float S = FTactixUtilitySelector::ScoreConsiderations(Cons, 1, Ctx);
	// N=1 -> ModFactor = 0, no compensation; result == raw input.
	EXPECT_NEAR(S, 0.7f, 1e-6f);
}
