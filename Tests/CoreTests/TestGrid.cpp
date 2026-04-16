// Copyright Sleak Software. All Rights Reserved.
//
// Tests for FTactixGrid — a flat 2D grid with world<->cell helpers. Focus
// areas: correct row-major indexing, floor-based WorldToCell (so negatives
// work), CellCenterToWorld roundtrip, and 8-connected neighbour queries at
// corners, edges, and the interior.

#include "Foundation/TactixGrid.h"

#include <gtest/gtest.h>

#include <cstddef>

TEST(TactixGrid, DimensionsAndCellCount)
{
    Tactix::FTactixGrid<int> Grid(4, 3, 10.0f);
    EXPECT_EQ(Grid.GetWidth(),     4);
    EXPECT_EQ(Grid.GetHeight(),    3);
    EXPECT_FLOAT_EQ(Grid.GetCellSize(), 10.0f);
    EXPECT_EQ(Grid.GetCellCount(), 12u);
}

TEST(TactixGrid, InBoundsGuardsEdges)
{
    Tactix::FTactixGrid<int> Grid(2, 2, 1.0f);
    EXPECT_TRUE (Grid.InBounds(0, 0));
    EXPECT_TRUE (Grid.InBounds(1, 1));
    EXPECT_FALSE(Grid.InBounds(-1, 0));
    EXPECT_FALSE(Grid.InBounds(2,  0));
    EXPECT_FALSE(Grid.InBounds(0,  2));
}

TEST(TactixGrid, WorldToCellHandlesNegativesViaFloor)
{
    Tactix::FTactixGrid<int> Grid(10, 10, 1.0f);
    int X = 0, Y = 0;

    Grid.WorldToCell({0.5f, 0.5f}, X, Y);
    EXPECT_EQ(X, 0);
    EXPECT_EQ(Y, 0);

    Grid.WorldToCell({-0.5f, -0.5f}, X, Y);
    // floor(-0.5) == -1. This is crucial for signed-origin worlds.
    EXPECT_EQ(X, -1);
    EXPECT_EQ(Y, -1);

    Grid.WorldToCell({9.999f, 0.001f}, X, Y);
    EXPECT_EQ(X, 9);
    EXPECT_EQ(Y, 0);
}

TEST(TactixGrid, CellCenterToWorldRoundtrips)
{
    Tactix::FTactixGrid<int> Grid(5, 5, 2.0f, Tactix::FTactixVec2{10.0f, 20.0f});

    for (int x = 0; x < 5; ++x)
    {
        for (int y = 0; y < 5; ++y)
        {
            const Tactix::FTactixVec2 Center = Grid.CellCenterToWorld(x, y);
            int BackX = 0, BackY = 0;
            Grid.WorldToCell(Center, BackX, BackY);
            EXPECT_EQ(BackX, x);
            EXPECT_EQ(BackY, y);
        }
    }
}

TEST(TactixGrid, ReadWriteAtStoresRowMajor)
{
    Tactix::FTactixGrid<int> Grid(3, 3, 1.0f);
    Grid.Fill(0);
    Grid.At(2, 1) = 42;

    EXPECT_EQ(Grid.At(2, 1), 42);
    // Row-major: (x, y) -> y * Width + x
    EXPECT_EQ(Grid.Index(2, 1), 1u * 3u + 2u);
    EXPECT_EQ(Grid[Grid.Index(2, 1)], 42);
}

TEST(TactixGrid, GetNeighboursInteriorEdgeCorner)
{
    Tactix::FTactixGrid<int> Grid(3, 3, 1.0f);
    std::size_t Idx[8];

    // Interior cell (1,1) has 8 neighbours.
    EXPECT_EQ(Grid.GetNeighbours(1, 1, Idx), 8);

    // Edge cell (1,0) has 5 neighbours.
    EXPECT_EQ(Grid.GetNeighbours(1, 0, Idx), 5);

    // Corner cell (0,0) has 3 neighbours.
    EXPECT_EQ(Grid.GetNeighbours(0, 0, Idx), 3);

    // Corner cell (2,2) — the other corner — also has 3.
    EXPECT_EQ(Grid.GetNeighbours(2, 2, Idx), 3);
}

TEST(TactixGrid, FillWritesEveryCell)
{
    Tactix::FTactixGrid<int> Grid(4, 4, 1.0f);
    Grid.Fill(-7);
    for (std::size_t i = 0; i < Grid.GetCellCount(); ++i)
    {
        EXPECT_EQ(Grid[i], -7);
    }
}
