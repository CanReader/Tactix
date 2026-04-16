// Copyright Sleak Software. All Rights Reserved.
//
// Tests for FTactixPerceptionMemory — per-agent ring buffer with half-life
// decay. Verifies insert/overwrite order, 2^(-age/half_life) decay math,
// floor zeroing, and channel-filtered queries.

#include "Perception/FTactixPerceptionMemory.h"

#include <gtest/gtest.h>

#include <cmath>

namespace
{
    constexpr uint16_t kChannelSight  = 1;
    constexpr uint16_t kChannelSound  = 2;

    Tactix::FTactixStimulus MakeStim(uint16_t Ch, float Intensity, double TsSeconds,
                                     Tactix::FTactixVec3 Origin = {})
    {
        Tactix::FTactixStimulus S;
        S.ChannelId = Ch;
        S.Intensity = Intensity;
        S.Timestamp = TsSeconds;
        S.Origin    = Origin;
        S.Kind      = Tactix::ETactixStimulusKind::Sight;
        return S;
    }
}

TEST(TactixPerceptionMemory, InsertIncreasesSizeUpToCapacity)
{
    Tactix::FTactixPerceptionMemory<4> Mem;
    EXPECT_EQ(Mem.Num(), 0u);

    Mem.Insert(MakeStim(kChannelSight, 1.0f, 0.0));
    EXPECT_EQ(Mem.Num(), 1u);
    Mem.Insert(MakeStim(kChannelSight, 1.0f, 0.1));
    Mem.Insert(MakeStim(kChannelSight, 1.0f, 0.2));
    Mem.Insert(MakeStim(kChannelSight, 1.0f, 0.3));
    EXPECT_EQ(Mem.Num(), 4u);

    // Overflowing must not grow the ring — oldest is overwritten.
    Mem.Insert(MakeStim(kChannelSight, 1.0f, 0.4));
    EXPECT_EQ(Mem.Num(), 4u);
}

TEST(TactixPerceptionMemory, OverflowOverwritesOldest)
{
    Tactix::FTactixPerceptionMemory<4> Mem;
    for (int i = 0; i < 6; ++i)
    {
        Mem.Insert(MakeStim(kChannelSight, 1.0f, static_cast<double>(i)));
    }

    // After 6 inserts into capacity 4, the 2 oldest (ts=0, ts=1) are gone.
    // The most recent should be ts=5.
    const auto* Latest = Mem.LatestForChannel(kChannelSight);
    ASSERT_NE(Latest, nullptr);
    EXPECT_DOUBLE_EQ(Latest->Timestamp, 5.0);
}

TEST(TactixPerceptionMemory, DecayHalvesIntensityAtHalfLife)
{
    // A single Decay() call at Age == HalfLife must halve the intensity.
    // (Repeated calls compound the decay against the original timestamp —
    //  that is a separate, documented semantic exercised in the next test.)
    Tactix::FTactixPerceptionMemory<4> Mem;
    Mem.Insert(MakeStim(kChannelSight, 1.0f, /*ts=*/0.0));

    Mem.Decay(/*NowSeconds=*/5.0, /*DefaultHalfLife=*/5.0f, /*Floor=*/0.0f);
    const auto* S = Mem.LatestForChannel(kChannelSight);
    ASSERT_NE(S, nullptr);
    EXPECT_NEAR(S->Intensity, 0.5f, 1e-4f);
}

TEST(TactixPerceptionMemory, DecayAtQuarterLifeMatchesExp2)
{
    Tactix::FTactixPerceptionMemory<4> Mem;
    Mem.Insert(MakeStim(kChannelSight, 1.0f, /*ts=*/0.0));

    // 1.25s with a 5s half-life -> intensity *= 2^(-0.25) ~= 0.8409.
    Mem.Decay(/*NowSeconds=*/1.25, /*DefaultHalfLife=*/5.0f, /*Floor=*/0.0f);
    const auto* S = Mem.LatestForChannel(kChannelSight);
    ASSERT_NE(S, nullptr);
    EXPECT_NEAR(S->Intensity, 0.8408964f, 1e-4f);
}

TEST(TactixPerceptionMemory, DecayFloorZeroesLowIntensity)
{
    Tactix::FTactixPerceptionMemory<4> Mem;
    Mem.Insert(MakeStim(kChannelSight, 1.0f, /*ts=*/0.0));

    // After many half-lives we're well below the floor.
    Mem.Decay(/*NowSeconds=*/60.0, /*DefaultHalfLife=*/5.0f, /*Floor=*/0.1f);
    EXPECT_EQ(Mem.LatestForChannel(kChannelSight), nullptr);
}

TEST(TactixPerceptionMemory, StrongestForChannelIgnoresOtherChannels)
{
    Tactix::FTactixPerceptionMemory<8> Mem;
    Mem.Insert(MakeStim(kChannelSight, 0.3f, 0.0));
    Mem.Insert(MakeStim(kChannelSound, 0.9f, 0.1)); // higher, but different channel
    Mem.Insert(MakeStim(kChannelSight, 0.7f, 0.2));

    const auto* Sight = Mem.StrongestForChannel(kChannelSight);
    ASSERT_NE(Sight, nullptr);
    EXPECT_FLOAT_EQ(Sight->Intensity, 0.7f);

    const auto* Sound = Mem.StrongestForChannel(kChannelSound);
    ASSERT_NE(Sound, nullptr);
    EXPECT_FLOAT_EQ(Sound->Intensity, 0.9f);
}

TEST(TactixPerceptionMemory, RecentReturnsMostRecentFirstForChannel)
{
    Tactix::FTactixPerceptionMemory<8> Mem;
    Mem.Insert(MakeStim(kChannelSight, 0.1f, 0.0));
    Mem.Insert(MakeStim(kChannelSound, 0.9f, 0.1));
    Mem.Insert(MakeStim(kChannelSight, 0.2f, 0.2));
    Mem.Insert(MakeStim(kChannelSight, 0.3f, 0.3));

    Tactix::FTactixStimulus Out[4];
    const std::size_t N = Mem.Recent(kChannelSight, Out, 4);
    EXPECT_EQ(N, 3u);

    // Newest-first: ts=0.3, then 0.2, then 0.0.
    EXPECT_DOUBLE_EQ(Out[0].Timestamp, 0.3);
    EXPECT_DOUBLE_EQ(Out[1].Timestamp, 0.2);
    EXPECT_DOUBLE_EQ(Out[2].Timestamp, 0.0);
}

TEST(TactixPerceptionMemory, ClearResetsAllState)
{
    Tactix::FTactixPerceptionMemory<4> Mem;
    Mem.Insert(MakeStim(kChannelSight, 1.0f, 0.0));
    EXPECT_EQ(Mem.Num(), 1u);
    Mem.Clear();
    EXPECT_EQ(Mem.Num(), 0u);
    EXPECT_EQ(Mem.LatestForChannel(kChannelSight), nullptr);
}
