// Copyright Sleak Software. All Rights Reserved.

#include <gtest/gtest.h>

#include "Squad/FTactixSquad.h"
#include "Squad/FTactixFormation.h"

using namespace Tactix;

namespace
{
	FTactixHandle<ITactixAgent> Agent(uint32_t Idx) { return FTactixHandle<ITactixAgent>{Idx, 1u}; }
}

TEST(Squad, AddRejectsDuplicatesAndOverflow)
{
	FTactixSquad<2> S;
	EXPECT_TRUE (S.Add(Agent(1), ETactixSquadRole::Leader));
	EXPECT_FALSE(S.Add(Agent(1), ETactixSquadRole::Flanker));   // duplicate
	EXPECT_TRUE (S.Add(Agent(2), ETactixSquadRole::Assault));
	EXPECT_FALSE(S.Add(Agent(3), ETactixSquadRole::Scout));     // over capacity
	EXPECT_EQ   (S.Num(), 2u);
}

TEST(Squad, RemoveSwapsAndClearsLeader)
{
	FTactixSquad<4> S;
	S.Add(Agent(1), ETactixSquadRole::Leader);
	S.Add(Agent(2), ETactixSquadRole::Assault);
	S.SetLeader(Agent(1));

	EXPECT_TRUE(S.Remove(Agent(1)));
	EXPECT_EQ (S.Num(), 1u);
	EXPECT_FALSE(S.GetLeader().IsValid());
	EXPECT_NE(S.Find(Agent(2)), nullptr);
}

TEST(Squad, AssignRoleAndSlot)
{
	FTactixSquad<4> S;
	S.Add(Agent(1), ETactixSquadRole::Assault);

	EXPECT_TRUE(S.AssignRole(Agent(1), ETactixSquadRole::Leader));
	EXPECT_EQ  (S.Find(Agent(1))->Role, ETactixSquadRole::Leader);

	EXPECT_TRUE(S.AssignSlot(Agent(1), 3u));
	EXPECT_EQ  (S.Find(Agent(1))->SlotIndex, 3u);
}

TEST(Squad, SetLeaderIgnoresNonMember)
{
	FTactixSquad<4> S;
	S.Add(Agent(1), ETactixSquadRole::Assault);

	S.SetLeader(Agent(99));
	EXPECT_FALSE(S.GetLeader().IsValid());

	S.SetLeader(Agent(1));
	EXPECT_EQ(S.GetLeader(), Agent(1));
}

TEST(Squad, TacticPersistsUntilChanged)
{
	FTactixSquad<4> S;
	EXPECT_EQ(S.GetTactic(), ETactixSquadTactic::Idle);
	S.SetTactic(ETactixSquadTactic::Flank);
	EXPECT_EQ(S.GetTactic(), ETactixSquadTactic::Flank);
}

TEST(Squad, CompactSlotsReassignsInOrder)
{
	FTactixSquad<4> S;
	S.Add(Agent(1), ETactixSquadRole::Leader);
	S.Add(Agent(2), ETactixSquadRole::Assault);
	S.Add(Agent(3), ETactixSquadRole::Scout);
	S.Remove(Agent(2));

	S.CompactSlots();
	EXPECT_EQ(S.GetMember(0).SlotIndex, 0u);
	EXPECT_EQ(S.GetMember(1).SlotIndex, 1u);
}

TEST(Formation, LineAlternatesAroundLeader)
{
	FTactixFormation<5> F(ETactixFormationKind::Line, 5, 100.0f);

	// Leader forward = +X; line stretches across +-Y.
	const FTactixVec3 L{0, 0, 0};
	const FTactixVec3 Fwd{1, 0, 0};

	EXPECT_EQ(F.GetSlotWorldPosition(0, L, Fwd), FTactixVec3(0.0f, 0.0f, 0.0f));
	// Slot 1: Local Y = +Spacing -> world = L + Right * 100 = (0, 100, 0) in UE-left-hand.
	EXPECT_EQ(F.GetSlotWorldPosition(1, L, Fwd), FTactixVec3(0.0f, 100.0f, 0.0f));
	EXPECT_EQ(F.GetSlotWorldPosition(2, L, Fwd), FTactixVec3(0.0f, -100.0f, 0.0f));
}

TEST(Formation, ColumnTrailsLeader)
{
	FTactixFormation<4> F(ETactixFormationKind::Column, 4, 50.0f);
	const FTactixVec3 L{500.0f, 0.0f, 0.0f};
	const FTactixVec3 Fwd{1, 0, 0};

	EXPECT_EQ(F.GetSlotWorldPosition(0, L, Fwd), FTactixVec3(500.0f,  0.0f, 0.0f));
	EXPECT_EQ(F.GetSlotWorldPosition(1, L, Fwd), FTactixVec3(450.0f,  0.0f, 0.0f));
	EXPECT_EQ(F.GetSlotWorldPosition(2, L, Fwd), FTactixVec3(400.0f,  0.0f, 0.0f));
	EXPECT_EQ(F.GetSlotWorldPosition(3, L, Fwd), FTactixVec3(350.0f,  0.0f, 0.0f));
}

TEST(Formation, WedgeLeaderAtPoint)
{
	FTactixFormation<5> F(ETactixFormationKind::Wedge, 5, 100.0f);

	const FTactixVec3 L{0, 0, 0};
	const FTactixVec3 Fwd{1, 0, 0};

	// Slot 0 at leader.
	EXPECT_EQ(F.GetSlotWorldPosition(0, L, Fwd), FTactixVec3(0.0f, 0.0f, 0.0f));
	// Slots 1,2 one step back, to right/left.
	const FTactixVec3 P1 = F.GetSlotWorldPosition(1, L, Fwd);
	const FTactixVec3 P2 = F.GetSlotWorldPosition(2, L, Fwd);
	EXPECT_FLOAT_EQ(P1.X, -100.0f);
	EXPECT_FLOAT_EQ(P2.X, -100.0f);
	EXPECT_NE(P1, P2);
	EXPECT_NEAR(P1.Y, -P2.Y, 1e-4f);
}

TEST(Formation, CustomSlotOverridesPattern)
{
	FTactixFormation<4> F(ETactixFormationKind::Line, 3, 100.0f);
	F.SetCustomSlot(1, {10.0f, 20.0f, 30.0f});
	EXPECT_EQ(F.GetKind(), ETactixFormationKind::Custom);

	const FTactixVec3 L{0, 0, 0};
	const FTactixVec3 Fwd{1, 0, 0};
	// Forward(1,0,0), Right(0,1,0), Up(0,0,1) -> world = 10*F + 20*R + 30*Up.
	EXPECT_EQ(F.GetSlotWorldPosition(1, L, Fwd), FTactixVec3(10.0f, 20.0f, 30.0f));
}

TEST(Formation, RotatesWithLeaderForward)
{
	FTactixFormation<3> F(ETactixFormationKind::Column, 3, 100.0f);
	const FTactixVec3 L{0, 0, 0};
	const FTactixVec3 Fwd{0.0f, 1.0f, 0.0f};   // facing +Y

	// Column is -X locally; with forward=+Y, that becomes -Y in world.
	EXPECT_EQ(F.GetSlotWorldPosition(1, L, Fwd), FTactixVec3(0.0f, -100.0f, 0.0f));
	EXPECT_EQ(F.GetSlotWorldPosition(2, L, Fwd), FTactixVec3(0.0f, -200.0f, 0.0f));
}
