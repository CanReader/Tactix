// Copyright Sleak Software. All Rights Reserved.
//
// Tests for FTactixArena — the bump allocator used by the GOAP / HTN planners
// for per-pass scratch. Verifies alignment, capacity exhaustion, Reset(), and
// move semantics.

#include "Foundation/TactixArena.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

TEST(TactixArena, EmplaceAndAlloc)
{
    Tactix::FTactixArena Arena(1024);
    EXPECT_EQ(Arena.GetUsed(), 0u);
    EXPECT_EQ(Arena.GetCapacity(), 1024u);

    int* A = Arena.Emplace<int>(42);
    ASSERT_NE(A, nullptr);
    EXPECT_EQ(*A, 42);
    EXPECT_GE(Arena.GetUsed(), sizeof(int));

    double* B = Arena.Emplace<double>(3.14);
    ASSERT_NE(B, nullptr);
    EXPECT_DOUBLE_EQ(*B, 3.14);
}

TEST(TactixArena, AlignmentHonoredEvenWithMisalignedCursor)
{
    Tactix::FTactixArena Arena(128);

    // Consume 1 byte so the next allocation starts misaligned.
    void* One = Arena.Alloc(1, 1);
    ASSERT_NE(One, nullptr);
    EXPECT_EQ(Arena.GetUsed(), 1u);

    // A 16-byte-aligned allocation must be placed at the next multiple of 16.
    void* Aligned = Arena.Alloc(32, 16);
    ASSERT_NE(Aligned, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(Aligned) & 0xF, 0u);
}

TEST(TactixArena, OutOfSpaceReturnsNullptrWithoutCorrupting)
{
    Tactix::FTactixArena Arena(16);
    EXPECT_NE(Arena.Alloc(8, 1), nullptr);
    EXPECT_NE(Arena.Alloc(8, 1), nullptr);
    EXPECT_EQ(Arena.GetRemaining(), 0u);

    // Next alloc fails but internal state stays consistent.
    EXPECT_EQ(Arena.Alloc(1, 1), nullptr);
    EXPECT_EQ(Arena.GetUsed(), Arena.GetCapacity());
}

TEST(TactixArena, ResetRewindsCursor)
{
    Tactix::FTactixArena Arena(256);

    for (int i = 0; i < 10; ++i)
    {
        int* Value = Arena.Emplace<int>(i);
        ASSERT_NE(Value, nullptr);
    }
    EXPECT_GT(Arena.GetUsed(), 0u);

    Arena.Reset();
    EXPECT_EQ(Arena.GetUsed(), 0u);
    EXPECT_EQ(Arena.GetRemaining(), Arena.GetCapacity());

    // Reset lets us reuse the full capacity.
    void* First = Arena.Alloc(200, 1);
    EXPECT_NE(First, nullptr);
}

TEST(TactixArena, EmplaceArrayUninitialised)
{
    Tactix::FTactixArena Arena(256);
    int* Array = Arena.EmplaceArrayUninitialised<int>(10);
    ASSERT_NE(Array, nullptr);

    // The array isn't initialised by the arena; that's by contract. We simply
    // verify we can write to all 10 cells without touching anyone else.
    for (int i = 0; i < 10; ++i) Array[i] = i * 2;
    for (int i = 0; i < 10; ++i) EXPECT_EQ(Array[i], i * 2);

    // A zero-length request is a documented no-op that returns nullptr.
    EXPECT_EQ(Arena.EmplaceArrayUninitialised<int>(0), nullptr);
}

TEST(TactixArena, MoveTransfersBuffer)
{
    Tactix::FTactixArena A(64);
    int* X = A.Emplace<int>(99);
    ASSERT_NE(X, nullptr);
    const std::size_t Used = A.GetUsed();

    Tactix::FTactixArena B(std::move(A));
    EXPECT_EQ(B.GetUsed(), Used);
    EXPECT_EQ(A.GetCapacity(), 0u);      // moved-from is empty/safe-to-destroy
    EXPECT_EQ(*X, 99);                   // pointer remains valid in B
}
