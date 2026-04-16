// Copyright Sleak Software. All Rights Reserved.

#include <gtest/gtest.h>

#include "GOAP/FTactixWorldState.h"
#include "GOAP/ITactixGOAPAction.h"
#include "GOAP/FTactixGOAPPlanner.h"
#include "Agent/FTactixAgentContext.h"
#include "Foundation/TactixArena.h"

using namespace Tactix;

namespace
{
	// 0 = HasAmmo, 1 = HasWeapon, 2 = EnemyVisible, 3 = EnemyDead, 4 = AtCover, 5 = AtAmmoCrate
	constexpr uint32_t Fact_HasAmmo      = 0;
	constexpr uint32_t Fact_HasWeapon    = 1;
	constexpr uint32_t Fact_EnemyVisible = 2;
	constexpr uint32_t Fact_EnemyDead    = 3;
	constexpr uint32_t Fact_AtCover      = 4;
	constexpr uint32_t Fact_AtAmmoCrate  = 5;

	struct SimpleAction : public ITactixGOAPAction
	{
		FTactixWorldState Pre{};
		FTactixWorldState Eff{};
		float             Cost{1.0f};
		const char*       Name{""};

		FTactixWorldState GetPreconditions(const FTactixAgentContext&) const override { return Pre; }
		FTactixWorldState GetEffects      (const FTactixAgentContext&) const override { return Eff; }
		float             GetCost         (const FTactixAgentContext&) const override { return Cost; }
		bool              IsApplicable    (const FTactixAgentContext&) const override { return true; }
		void              Execute         (FTactixAgentContext&)             override {}
	};
}

TEST(WorldState, SatisfiesIgnoresUndefinedBits)
{
	FTactixWorldState S;
	S.Set(0, true);
	S.Set(1, false);

	FTactixWorldState Req;
	Req.Set(0, true);   // only cares about bit 0

	EXPECT_TRUE(S.Satisfies(Req));
}

TEST(WorldState, SatisfiesFailsOnMismatch)
{
	FTactixWorldState S;
	S.Set(0, false);

	FTactixWorldState Req;
	Req.Set(0, true);

	EXPECT_FALSE(S.Satisfies(Req));
}

TEST(WorldState, ApplyOverlaysDelta)
{
	FTactixWorldState S;
	S.Set(0, false);
	S.Set(1, true);

	FTactixWorldState Delta;
	Delta.Set(0, true);
	Delta.Set(2, true);

	S.Apply(Delta);
	EXPECT_TRUE (S.Get(0));
	EXPECT_TRUE (S.Get(1));
	EXPECT_TRUE (S.Get(2));
}

TEST(WorldState, DistanceCountsMismatchedBits)
{
	FTactixWorldState S;
	S.Set(0, false); S.Set(1, false); S.Set(2, false);

	FTactixWorldState Target;
	Target.Set(0, true); Target.Set(1, true); Target.Set(2, false);

	EXPECT_EQ(S.Distance(Target), 2u);
}

TEST(WorldState, HashIsStableAndDistinct)
{
	FTactixWorldState A; A.Set(0, true);  A.Set(1, false);
	FTactixWorldState B; B.Set(0, true);  B.Set(1, false);
	FTactixWorldState C; C.Set(0, false); C.Set(1, false);

	EXPECT_EQ(A.Hash(), B.Hash());
	EXPECT_NE(A.Hash(), C.Hash());
	EXPECT_EQ(A, B);
	EXPECT_NE(A, C);
}

TEST(GOAPPlanner, EmptyPlanWhenGoalAlreadyMet)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(64 * 1024);

	FTactixWorldState Start; Start.Set(Fact_EnemyDead, true);
	FTactixWorldState Goal;  Goal.Set (Fact_EnemyDead, true);

	ITactixGOAPAction* Actions[] = { nullptr };
	const FTactixGOAPPlan P = FTactixGOAPPlanner::Plan(Start, Goal, Actions, 0, Ctx, Arena);

	EXPECT_TRUE(P.bValid);
	EXPECT_EQ(P.Count, 0u);
	EXPECT_FLOAT_EQ(P.Cost, 0.0f);
}

TEST(GOAPPlanner, SingleActionPlan)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(64 * 1024);

	SimpleAction Shoot;
	Shoot.Name = "Shoot";
	Shoot.Pre.Set(Fact_HasAmmo, true);
	Shoot.Pre.Set(Fact_EnemyVisible, true);
	Shoot.Eff.Set(Fact_EnemyDead, true);
	Shoot.Cost = 1.0f;

	FTactixWorldState Start;
	Start.Set(Fact_HasAmmo, true);
	Start.Set(Fact_EnemyVisible, true);
	Start.Set(Fact_EnemyDead, false);

	FTactixWorldState Goal;
	Goal.Set(Fact_EnemyDead, true);

	ITactixGOAPAction* Actions[] = { &Shoot };
	const FTactixGOAPPlan P = FTactixGOAPPlanner::Plan(Start, Goal, Actions, 1, Ctx, Arena);

	ASSERT_TRUE(P.bValid);
	ASSERT_EQ(P.Count, 1u);
	EXPECT_EQ(P.Steps[0], &Shoot);
	EXPECT_FLOAT_EQ(P.Cost, 1.0f);
}

TEST(GOAPPlanner, MultiStepPlanChainsPreconditions)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(128 * 1024);

	// PickUpAmmo: Pre {HasWeapon}, Eff {HasAmmo}
	SimpleAction PickUp;
	PickUp.Pre.Set(Fact_HasWeapon, true);
	PickUp.Eff.Set(Fact_HasAmmo,  true);
	PickUp.Cost = 2.0f;

	// Shoot: Pre {HasAmmo, EnemyVisible}, Eff {EnemyDead}
	SimpleAction Shoot;
	Shoot.Pre.Set(Fact_HasAmmo, true);
	Shoot.Pre.Set(Fact_EnemyVisible, true);
	Shoot.Eff.Set(Fact_EnemyDead, true);
	Shoot.Cost = 1.0f;

	FTactixWorldState Start;
	Start.Set(Fact_HasWeapon, true);
	Start.Set(Fact_HasAmmo,   false);
	Start.Set(Fact_EnemyVisible, true);
	Start.Set(Fact_EnemyDead, false);

	FTactixWorldState Goal;
	Goal.Set(Fact_EnemyDead, true);

	ITactixGOAPAction* Actions[] = { &PickUp, &Shoot };
	const FTactixGOAPPlan P = FTactixGOAPPlanner::Plan(Start, Goal, Actions, 2, Ctx, Arena);

	ASSERT_TRUE(P.bValid);
	ASSERT_EQ(P.Count, 2u);
	EXPECT_EQ(P.Steps[0], &PickUp);
	EXPECT_EQ(P.Steps[1], &Shoot);
	EXPECT_FLOAT_EQ(P.Cost, 3.0f);
}

TEST(GOAPPlanner, PrefersCheaperPathWhenBothReachGoal)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(128 * 1024);

	SimpleAction Direct;
	Direct.Pre.Set(Fact_HasAmmo, true);
	Direct.Eff.Set(Fact_EnemyDead, true);
	Direct.Cost = 5.0f;

	SimpleAction Detour;
	Detour.Pre.Set(Fact_HasAmmo, true);
	Detour.Eff.Set(Fact_EnemyDead, true);
	Detour.Cost = 1.0f;

	FTactixWorldState Start; Start.Set(Fact_HasAmmo, true); Start.Set(Fact_EnemyDead, false);
	FTactixWorldState Goal;  Goal.Set (Fact_EnemyDead, true);

	ITactixGOAPAction* Actions[] = { &Direct, &Detour };
	const FTactixGOAPPlan P = FTactixGOAPPlanner::Plan(Start, Goal, Actions, 2, Ctx, Arena);

	ASSERT_TRUE(P.bValid);
	ASSERT_EQ(P.Count, 1u);
	EXPECT_EQ(P.Steps[0], &Detour);
	EXPECT_FLOAT_EQ(P.Cost, 1.0f);
}

TEST(GOAPPlanner, FailsWhenNoActionReachesGoal)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(64 * 1024);

	SimpleAction Unrelated;
	Unrelated.Pre.Set(Fact_HasWeapon, true);
	Unrelated.Eff.Set(Fact_AtCover,   true);

	FTactixWorldState Start;
	Start.Set(Fact_HasWeapon, true);
	Start.Set(Fact_EnemyDead, false);

	FTactixWorldState Goal;
	Goal.Set(Fact_EnemyDead, true);

	ITactixGOAPAction* Actions[] = { &Unrelated };
	const FTactixGOAPPlan P = FTactixGOAPPlanner::Plan(Start, Goal, Actions, 1, Ctx, Arena);

	EXPECT_FALSE(P.bValid);
}
