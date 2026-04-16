// Copyright Sleak Software. All Rights Reserved.

#include <gtest/gtest.h>

#include "Spatial/FTactixInfluenceMap.h"

using namespace Tactix;

TEST(InfluenceMap, ConstructionAndZeroInit)
{
	FTactixInfluenceMap<2> Map(8, 8, 100.0f);
	EXPECT_EQ(Map.GetWidth(),  8);
	EXPECT_EQ(Map.GetHeight(), 8);
	EXPECT_FLOAT_EQ(Map.GetCellSize(), 100.0f);
	EXPECT_FLOAT_EQ(Map.SampleNearest(0, {50.0f, 50.0f}), 0.0f);
	EXPECT_FLOAT_EQ(Map.SampleNearest(1, {50.0f, 50.0f}), 0.0f);
}

TEST(InfluenceMap, StampFillsCenterStrongest)
{
	FTactixInfluenceMap<1> Map(16, 16, 50.0f);
	Map.Stamp(0, {400.0f, 400.0f}, 200.0f, 1.0f);

	const float Center = Map.SampleNearest(0, {400.0f, 400.0f});
	const float Edge   = Map.SampleNearest(0, {400.0f + 180.0f, 400.0f});
	EXPECT_GT(Center, Edge);
	EXPECT_GT(Center, 0.8f);
}

TEST(InfluenceMap, StampFalloffOutsideRadiusIsZero)
{
	FTactixInfluenceMap<1> Map(16, 16, 50.0f);
	Map.Stamp(0, {400.0f, 400.0f}, 100.0f, 1.0f);

	EXPECT_FLOAT_EQ(Map.SampleNearest(0, {700.0f, 400.0f}), 0.0f);
}

TEST(InfluenceMap, StampsAccumulate)
{
	FTactixInfluenceMap<1> Map(8, 8, 100.0f);
	Map.Stamp(0, {400.0f, 400.0f}, 150.0f, 0.5f);
	const float First = Map.SampleNearest(0, {400.0f, 400.0f});

	Map.Stamp(0, {400.0f, 400.0f}, 150.0f, 0.5f);
	const float Second = Map.SampleNearest(0, {400.0f, 400.0f});

	EXPECT_GT(Second, First);
	EXPECT_NEAR(Second, 2.0f * First, 1e-4f);
}

TEST(InfluenceMap, DecayHalvesAtHalfRate)
{
	FTactixInfluenceMap<1> Map(4, 4, 100.0f);
	Map.Stamp(0, {200.0f, 200.0f}, 150.0f, 1.0f);

	const float Before = Map.SampleNearest(0, {200.0f, 200.0f});
	Map.Decay(0, 0.5f);
	const float After = Map.SampleNearest(0, {200.0f, 200.0f});

	EXPECT_NEAR(After, Before * 0.5f, 1e-5f);
}

TEST(InfluenceMap, DecayFullRateClearsChannel)
{
	FTactixInfluenceMap<1> Map(4, 4, 100.0f);
	Map.Stamp(0, {200.0f, 200.0f}, 150.0f, 1.0f);
	Map.Decay(0, 1.0f);
	EXPECT_FLOAT_EQ(Map.SampleNearest(0, {200.0f, 200.0f}), 0.0f);
}

TEST(InfluenceMap, ClearResetsChannelOnly)
{
	FTactixInfluenceMap<2> Map(4, 4, 100.0f);
	Map.Stamp(0, {200.0f, 200.0f}, 150.0f, 1.0f);
	Map.Stamp(1, {200.0f, 200.0f}, 150.0f, 1.0f);

	Map.Clear(0);
	EXPECT_FLOAT_EQ(Map.SampleNearest(0, {200.0f, 200.0f}), 0.0f);
	EXPECT_GT     (Map.SampleNearest(1, {200.0f, 200.0f}), 0.0f);
}

TEST(InfluenceMap, ChannelsAreIndependent)
{
	FTactixInfluenceMap<3> Map(4, 4, 100.0f);
	Map.Stamp(1, {100.0f, 100.0f}, 120.0f, 1.0f);

	EXPECT_FLOAT_EQ(Map.SampleNearest(0, {100.0f, 100.0f}), 0.0f);
	EXPECT_GT     (Map.SampleNearest(1, {100.0f, 100.0f}), 0.0f);
	EXPECT_FLOAT_EQ(Map.SampleNearest(2, {100.0f, 100.0f}), 0.0f);
}

TEST(InfluenceMap, BilinearSampleInterpolatesBetweenCells)
{
	FTactixInfluenceMap<1> Map(4, 4, 100.0f);
	Map.ChannelData(0)[0] = 1.0f;   // cell (0,0)
	Map.ChannelData(0)[1] = 0.0f;   // cell (1,0)

	// cell (0,0) center is (50,50); cell (1,0) center is (150,50).
	// Bilinear sample at (100,50) = midway on X = 0.5.
	const float Mid = Map.SampleBilinear(0, {100.0f, 50.0f});
	EXPECT_NEAR(Mid, 0.5f, 1e-5f);
}
