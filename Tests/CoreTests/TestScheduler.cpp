// Copyright Sleak Software. All Rights Reserved.
//
// Tests for FTactixAgentScheduler — the min-heap priority queue that time-
// slices agent ticks against a frame microsecond budget. Covers registration
// and duplicate Register(), Unregister(), Collect() ordering by NextTickTime,
// budget-stop semantics with starvation prevention, and ReportTickCost.

#include "Agent/FTactixAgentScheduler.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

namespace
{
    using AgentHandle = Tactix::FTactixHandle<Tactix::ITactixAgent>;

    AgentHandle H(uint32_t Idx, uint32_t Gen = 1) { return AgentHandle{Idx, Gen}; }
}

TEST(TactixAgentScheduler, RegisterUnregister)
{
    Tactix::FTactixAgentScheduler S(4);
    EXPECT_EQ(S.Num(), 0u);

    Tactix::FTactixAgentScheduleEntry E;
    E.Agent          = H(1);
    E.NextTickTime   = 0.0;
    E.TickIntervalMs = 100;
    E.BudgetUs       = 50;

    EXPECT_TRUE (S.Register(E));
    EXPECT_EQ   (S.Num(), 1u);
    EXPECT_TRUE (S.Contains(H(1)));
    EXPECT_TRUE (S.Unregister(H(1)));
    EXPECT_EQ   (S.Num(), 0u);
    EXPECT_FALSE(S.Contains(H(1)));
}

TEST(TactixAgentScheduler, RegisterRejectsInvalidAgent)
{
    Tactix::FTactixAgentScheduler S;
    Tactix::FTactixAgentScheduleEntry E;
    E.Agent = AgentHandle{};      // invalid
    EXPECT_FALSE(S.Register(E));
    EXPECT_EQ(S.Num(), 0u);
}

TEST(TactixAgentScheduler, DuplicateRegisterReplacesInPlace)
{
    Tactix::FTactixAgentScheduler S;

    Tactix::FTactixAgentScheduleEntry E1;
    E1.Agent = H(7);
    E1.NextTickTime = 0.0;
    E1.BudgetUs = 100;
    EXPECT_TRUE(S.Register(E1));
    EXPECT_EQ(S.Num(), 1u);

    Tactix::FTactixAgentScheduleEntry E2 = E1;
    E2.BudgetUs = 200;
    E2.NextTickTime = 5.0;
    EXPECT_TRUE(S.Register(E2));
    EXPECT_EQ(S.Num(), 1u);  // still one, not two
}

TEST(TactixAgentScheduler, CollectDeliversOnlyDueAgentsInOrder)
{
    Tactix::FTactixAgentScheduler S;

    // Three agents with strictly increasing NextTickTime.
    for (uint32_t i = 0; i < 3; ++i)
    {
        Tactix::FTactixAgentScheduleEntry E;
        E.Agent          = H(i + 1);
        E.NextTickTime   = static_cast<double>(i);
        E.TickIntervalMs = 1000;
        E.BudgetUs       = 100;
        S.Register(E);
    }

    AgentHandle Buf[8] = {};

    // At t=0.5 only agent(1) is due; agents(2,3) are scheduled for t=1 and t=2.
    const std::size_t N1 = S.Collect(/*Now=*/0.5, /*FrameUs=*/10000, Buf, 8);
    EXPECT_EQ(N1, 1u);
    EXPECT_EQ(Buf[0], H(1));

    // At t=1.5, agent(2) is due; agent(1) was rescheduled to t=0.5 + 1.0 = 1.5
    // so it's ALSO due. Order should be earliest first.
    const std::size_t N2 = S.Collect(/*Now=*/1.5, /*FrameUs=*/10000, Buf, 8);
    EXPECT_EQ(N2, 2u);
    // Agent(1)'s NextTickTime was 0.5 + 1.0 = 1.5; agent(2)'s original
    // NextTickTime is 1.0. agent(2) comes first.
    EXPECT_EQ(Buf[0], H(2));
    EXPECT_EQ(Buf[1], H(1));
}

TEST(TactixAgentScheduler, CollectRespectsFrameBudgetAfterFirstDelivery)
{
    Tactix::FTactixAgentScheduler S;

    // Four agents, each claiming 500 µs. The frame budget is 1000 µs — that
    // means we can drain 2 agents (first is always allowed), but the third
    // would push us over and must be skipped.
    for (uint32_t i = 0; i < 4; ++i)
    {
        Tactix::FTactixAgentScheduleEntry E;
        E.Agent          = H(i + 1);
        E.NextTickTime   = 0.0;
        E.TickIntervalMs = 50;
        E.BudgetUs       = 500;
        E.LastSpentUs    = 500;
        S.Register(E);
    }

    AgentHandle Buf[8] = {};
    const std::size_t N = S.Collect(/*Now=*/1.0, /*FrameUs=*/1000, Buf, 8);
    EXPECT_EQ(N, 2u);
}

TEST(TactixAgentScheduler, CollectAlwaysDeliversFirstAgentEvenIfOversized)
{
    Tactix::FTactixAgentScheduler S;

    Tactix::FTactixAgentScheduleEntry Hog;
    Hog.Agent        = H(1);
    Hog.NextTickTime = 0.0;
    Hog.BudgetUs     = 10000;
    Hog.LastSpentUs  = 10000;     // way over any sane frame budget
    S.Register(Hog);

    AgentHandle Buf[4];
    const std::size_t N = S.Collect(/*Now=*/1.0, /*FrameUs=*/500, Buf, 4);
    EXPECT_EQ(N, 1u);             // starvation prevention: first is always run
    EXPECT_EQ(Buf[0], H(1));
}

TEST(TactixAgentScheduler, CollectReschedulesProcessedAgents)
{
    Tactix::FTactixAgentScheduler S;

    Tactix::FTactixAgentScheduleEntry E;
    E.Agent          = H(1);
    E.NextTickTime   = 0.0;
    E.TickIntervalMs = 1000;  // 1 s cadence
    E.BudgetUs       = 10;
    S.Register(E);

    AgentHandle Buf[4];
    EXPECT_EQ(S.Collect(0.5, 1000, Buf, 4), 1u);   // agent ticks at t=0.5
    // Next call at t=1.0 — agent was rescheduled to t=1.5, so it's NOT due.
    EXPECT_EQ(S.Collect(1.0, 1000, Buf, 4), 0u);
    // At t=2.0 the agent is due again.
    EXPECT_EQ(S.Collect(2.0, 1000, Buf, 4), 1u);
}

TEST(TactixAgentScheduler, ReportTickCostUpdatesBudget)
{
    Tactix::FTactixAgentScheduler S;

    Tactix::FTactixAgentScheduleEntry A;
    A.Agent        = H(1);
    A.NextTickTime = 0.0;
    A.BudgetUs     = 100;
    A.LastSpentUs  = 0;
    S.Register(A);

    Tactix::FTactixAgentScheduleEntry B;
    B.Agent        = H(2);
    B.NextTickTime = 0.0;
    B.BudgetUs     = 100;
    B.LastSpentUs  = 0;
    S.Register(B);

    // Hint that agent(2) actually ate 900 µs on its last run.
    S.ReportTickCost(H(2), 900);

    AgentHandle Buf[4];
    // Frame budget 400 µs. First agent is allowed (100 µs by BudgetUs fallback
    // since LastSpentUs==0 for agent(1)), second agent would push us to 1000
    // µs and must be dropped.
    const std::size_t N = S.Collect(1.0, 400, Buf, 4);
    EXPECT_EQ(N, 1u);
}
