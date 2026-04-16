// Copyright Sleak Software. All Rights Reserved.

#include <gtest/gtest.h>

#include "HTN/ITactixHTNTask.h"
#include "HTN/FTactixHTNPlanner.h"
#include "GOAP/FTactixWorldState.h"
#include "Agent/FTactixAgentContext.h"
#include "Foundation/TactixArena.h"

using namespace Tactix;

namespace
{
	constexpr uint32_t Fact_HasAmmo   = 0;
	constexpr uint32_t Fact_AtTarget  = 1;
	constexpr uint32_t Fact_TargetDead = 2;

	struct Primitive : public ITactixHTNPrimitive
	{
		FTactixWorldState RequiredState{};
		FTactixWorldState Effect{};
		const char*       Name{""};

		bool IsPrimitive() const override { return true; }

		bool IsApplicable(const FTactixWorldState& S, const FTactixAgentContext&) const override
		{
			return S.Satisfies(RequiredState);
		}

		FTactixWorldState GetEffects(const FTactixAgentContext&) const override { return Effect; }

		void Execute(FTactixAgentContext&) override {}
	};

	// Compound with two methods: first needs HasAmmo, second fires if empty.
	struct KillTargetCompound : public ITactixHTNCompound
	{
		Primitive* Shoot{nullptr};
		Primitive* Melee{nullptr};

		bool IsPrimitive() const override { return false; }

		uint32_t GetMethodCount() const override { return 2; }

		bool MethodApplies(uint32_t M, const FTactixWorldState& S, const FTactixAgentContext&) const override
		{
			if (M == 0) { FTactixWorldState R; R.Set(Fact_HasAmmo, true);  return S.Satisfies(R); }
			if (M == 1) { FTactixWorldState R; R.Set(Fact_HasAmmo, false); return S.Satisfies(R); }
			return false;
		}

		uint32_t GetMethodSubtasks(uint32_t M, ITactixHTNTask** Out, uint32_t Max) const override
		{
			if (Max == 0) return 0;
			if (M == 0) { Out[0] = Shoot; return 1; }
			if (M == 1) { Out[0] = Melee; return 1; }
			return 0;
		}
	};

	// Compound whose first method always decomposes but picks a failing primitive,
	// forcing rollback to the second method.
	struct RollbackCompound : public ITactixHTNCompound
	{
		Primitive* Failing{nullptr};
		Primitive* Succeeding{nullptr};

		bool IsPrimitive() const override { return false; }
		uint32_t GetMethodCount() const override { return 2; }

		bool MethodApplies(uint32_t, const FTactixWorldState&, const FTactixAgentContext&) const override
		{
			return true;
		}

		uint32_t GetMethodSubtasks(uint32_t M, ITactixHTNTask** Out, uint32_t Max) const override
		{
			if (Max == 0) return 0;
			Out[0] = (M == 0) ? static_cast<ITactixHTNTask*>(Failing) : static_cast<ITactixHTNTask*>(Succeeding);
			return 1;
		}
	};
}

TEST(HTNPlanner, SingleMethodDecomposition)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(64 * 1024);

	Primitive Shoot;
	Shoot.Name = "Shoot";
	Shoot.RequiredState.Set(Fact_HasAmmo, true);
	Shoot.Effect.Set(Fact_TargetDead, true);

	Primitive Melee;
	Melee.Name = "Melee";
	Melee.Effect.Set(Fact_TargetDead, true);

	KillTargetCompound Kill;
	Kill.Shoot = &Shoot;
	Kill.Melee = &Melee;

	FTactixWorldState State; State.Set(Fact_HasAmmo, true);

	const FTactixHTNPlan P = FTactixHTNPlanner::Plan(&Kill, State, Ctx, Arena);
	ASSERT_TRUE(P.bValid);
	ASSERT_EQ(P.Count, 1u);
	EXPECT_EQ(P.Steps[0], &Shoot);
}

TEST(HTNPlanner, FallsThroughToSecondMethodWhenFirstGuardFails)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(64 * 1024);

	Primitive Shoot;
	Shoot.RequiredState.Set(Fact_HasAmmo, true);

	Primitive Melee;  // no precondition

	KillTargetCompound Kill;
	Kill.Shoot = &Shoot;
	Kill.Melee = &Melee;

	FTactixWorldState State; State.Set(Fact_HasAmmo, false);

	const FTactixHTNPlan P = FTactixHTNPlanner::Plan(&Kill, State, Ctx, Arena);
	ASSERT_TRUE(P.bValid);
	ASSERT_EQ(P.Count, 1u);
	EXPECT_EQ(P.Steps[0], &Melee);
}

TEST(HTNPlanner, RollsBackStateOnFailedMethod)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(64 * 1024);

	// Failing primitive: never applicable.
	Primitive Failing;
	Failing.RequiredState.Set(Fact_AtTarget, true);  // but world says false

	Primitive Succeeding;
	Succeeding.Effect.Set(Fact_TargetDead, true);

	RollbackCompound Root;
	Root.Failing    = &Failing;
	Root.Succeeding = &Succeeding;

	FTactixWorldState State;   // Fact_AtTarget undefined -> Satisfies fails
	State.Set(Fact_AtTarget, false);

	const FTactixHTNPlan P = FTactixHTNPlanner::Plan(&Root, State, Ctx, Arena);
	ASSERT_TRUE(P.bValid);
	ASSERT_EQ(P.Count, 1u);
	EXPECT_EQ(P.Steps[0], &Succeeding);
}

TEST(HTNPlanner, FailsWhenAllMethodsFail)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(64 * 1024);

	Primitive F1, F2;
	F1.RequiredState.Set(Fact_HasAmmo, true);   // world has it false
	F2.RequiredState.Set(Fact_AtTarget, true);  // world has it false

	RollbackCompound Root;
	Root.Failing    = &F1;
	Root.Succeeding = &F2;

	FTactixWorldState State;
	State.Set(Fact_HasAmmo,  false);
	State.Set(Fact_AtTarget, false);

	const FTactixHTNPlan P = FTactixHTNPlanner::Plan(&Root, State, Ctx, Arena);
	EXPECT_FALSE(P.bValid);
}

TEST(HTNPlanner, AppliesEffectsForwardBetweenSiblingSubtasks)
{
	FTactixAgentContext Ctx{};
	FTactixArena        Arena(64 * 1024);

	// Method: [MoveToTarget, Shoot]. Shoot needs AtTarget; Move provides it.
	struct SeqCompound : public ITactixHTNCompound
	{
		Primitive* Move{nullptr};
		Primitive* Shoot{nullptr};

		bool IsPrimitive() const override { return false; }
		uint32_t GetMethodCount() const override { return 1; }
		bool MethodApplies(uint32_t, const FTactixWorldState&, const FTactixAgentContext&) const override { return true; }

		uint32_t GetMethodSubtasks(uint32_t, ITactixHTNTask** Out, uint32_t Max) const override
		{
			if (Max < 2) return 0;
			Out[0] = Move;
			Out[1] = Shoot;
			return 2;
		}
	};

	Primitive Move;
	Move.Effect.Set(Fact_AtTarget, true);

	Primitive Shoot;
	Shoot.RequiredState.Set(Fact_AtTarget, true);
	Shoot.Effect.Set(Fact_TargetDead, true);

	SeqCompound Root;
	Root.Move  = &Move;
	Root.Shoot = &Shoot;

	FTactixWorldState State;
	State.Set(Fact_AtTarget, false);

	const FTactixHTNPlan P = FTactixHTNPlanner::Plan(&Root, State, Ctx, Arena);
	ASSERT_TRUE(P.bValid);
	ASSERT_EQ(P.Count, 2u);
	EXPECT_EQ(P.Steps[0], &Move);
	EXPECT_EQ(P.Steps[1], &Shoot);
}
