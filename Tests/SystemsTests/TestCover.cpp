// Copyright Sleak Software. All Rights Reserved.

#include <gtest/gtest.h>

#include "Cover/FTactixCoverPoint.h"
#include "Cover/FTactixCoverSystem.h"

using namespace Tactix;

namespace
{
	FTactixHandle<ITactixAgent> MakeAgent(uint32_t Idx) { return FTactixHandle<ITactixAgent>{Idx, 1u}; }
}

TEST(CoverSystem, RegisterAndResolve)
{
	FTactixCoverSystem<8> Sys;
	FTactixCoverPoint P{};
	P.Position = { 100.0f, 0.0f, 0.0f };
	P.Normal   = { 1.0f, 0.0f, 0.0f };
	P.Height   = ETactixCoverHeight::High;

	const auto H = Sys.Register(P);
	ASSERT_TRUE(H.IsValid());
	ASSERT_EQ(Sys.Num(), 1u);

	const FTactixCoverPoint* Got = Sys.Get(H);
	ASSERT_NE(Got, nullptr);
	EXPECT_EQ(Got->Height, ETactixCoverHeight::High);
	EXPECT_FALSE(Got->IsClaimed());
}

TEST(CoverSystem, ClaimAndRelease)
{
	FTactixCoverSystem<4> Sys;
	FTactixCoverPoint P{};
	P.Position = { 50.0f, 0.0f, 0.0f };
	P.Normal   = { 1.0f, 0.0f, 0.0f };
	const auto H = Sys.Register(P);

	const auto A1 = MakeAgent(1);
	const auto A2 = MakeAgent(2);

	EXPECT_TRUE (Sys.Claim(H, A1));
	EXPECT_FALSE(Sys.Claim(H, A2));   // already claimed by A1
	EXPECT_TRUE (Sys.Claim(H, A1));   // idempotent for same agent

	EXPECT_FALSE(Sys.Release(H, A2)); // wrong agent
	EXPECT_TRUE (Sys.Release(H, A1));
	EXPECT_FALSE(Sys.Get(H)->IsClaimed());

	EXPECT_TRUE(Sys.Claim(H, A2));    // now A2 can take it
}

TEST(CoverSystem, QueryBestPicksHighestBlockingCover)
{
	FTactixCoverSystem<8> Sys;

	// Wall at X=500; agent at origin; threat further along +X.
	// Good cover: Position=(500,0,0), Normal=(-1,0,0): back to wall, threat is in +X -> -dot((1,0,0), (-1,0,0)) = 1.
	FTactixCoverPoint Good;
	Good.Position = { 500.0f, 0.0f, 0.0f };
	Good.Normal   = { -1.0f,  0.0f, 0.0f };
	Good.Height   = ETactixCoverHeight::High;

	// Bad cover: Normal points toward threat -> useless.
	FTactixCoverPoint Bad;
	Bad.Position = { 500.0f, 200.0f, 0.0f };
	Bad.Normal   = { 1.0f,   0.0f,   0.0f };

	const auto GoodH = Sys.Register(Good);
	const auto BadH  = Sys.Register(Bad);
	(void)BadH;

	FTactixCoverSystem<8>::FQuery Q{};
	Q.FromPosition   = { 0.0f,   0.0f, 0.0f };
	Q.ThreatPosition = { 1000.0f,0.0f, 0.0f };
	Q.MaxRange       = 2000.0f;
	Q.MinBlockDot    = 0.5f;

	const auto Pick = Sys.QueryBest(Q);
	EXPECT_EQ(Pick, GoodH);
}

TEST(CoverSystem, QuerySkipsClaimedPointsByDefault)
{
	FTactixCoverSystem<8> Sys;

	FTactixCoverPoint P;
	P.Position = { 500.0f, 0.0f, 0.0f };
	P.Normal   = { -1.0f,  0.0f, 0.0f };
	const auto H = Sys.Register(P);

	Sys.Claim(H, MakeAgent(5));

	FTactixCoverSystem<8>::FQuery Q{};
	Q.FromPosition   = { 0.0f, 0.0f, 0.0f };
	Q.ThreatPosition = { 1000.0f, 0.0f, 0.0f };
	Q.MaxRange       = 2000.0f;
	Q.Querier        = MakeAgent(42);  // different agent
	Q.bExcludeClaimed = true;

	EXPECT_FALSE(Sys.QueryBest(Q).IsValid());

	// Same querier as the claimer -> allowed.
	Q.Querier = MakeAgent(5);
	EXPECT_EQ(Sys.QueryBest(Q), H);
}

TEST(CoverSystem, QueryRespectsMaxRange)
{
	FTactixCoverSystem<8> Sys;

	FTactixCoverPoint Far;
	Far.Position = { 5000.0f, 0.0f, 0.0f };
	Far.Normal   = { -1.0f,   0.0f, 0.0f };
	Sys.Register(Far);

	FTactixCoverSystem<8>::FQuery Q{};
	Q.FromPosition   = { 0.0f, 0.0f, 0.0f };
	Q.ThreatPosition = { 10000.0f, 0.0f, 0.0f };
	Q.MaxRange       = 1000.0f;

	EXPECT_FALSE(Sys.QueryBest(Q).IsValid());
}

TEST(CoverSystem, UnregisterFreesSlot)
{
	FTactixCoverSystem<4> Sys;

	FTactixCoverPoint P{};
	P.Normal = { 1.0f, 0.0f, 0.0f };

	const auto H = Sys.Register(P);
	EXPECT_EQ(Sys.Num(), 1u);

	EXPECT_TRUE(Sys.Unregister(H));
	EXPECT_EQ(Sys.Num(), 0u);
	EXPECT_EQ(Sys.Get(H), nullptr);
}
