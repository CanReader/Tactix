// Copyright Sleak Software. All Rights Reserved.
//
// Tests for FTactixPool — the fixed-capacity, no-heap-on-hot-path allocator
// that backs every pooled object in Tactix. Verifies alloc/free semantics,
// generation bumping that invalidates stale handles, and that destructors fire
// for non-trivially-destructible T.

#include "Foundation/TactixPool.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace
{
    struct Widget
    {
        int X;
        int Y;
        Widget() = default;
        Widget(int InX, int InY) : X(InX), Y(InY) {}
    };

    // Tracks how many live instances exist so we can verify destructors run.
    struct LifetimeCounter
    {
        static inline int Alive = 0;
        std::string Label;

        LifetimeCounter() { ++Alive; }
        explicit LifetimeCounter(std::string InLabel) : Label(std::move(InLabel)) { ++Alive; }
        ~LifetimeCounter() { --Alive; }

        LifetimeCounter(const LifetimeCounter&)            = delete;
        LifetimeCounter& operator=(const LifetimeCounter&) = delete;
    };
}

TEST(TactixPool, AllocReturnsValidHandleAndDefaultState)
{
    Tactix::FTactixPool<Widget, 4> Pool;
    EXPECT_EQ(Pool.Num(), 0u);
    EXPECT_FALSE(Pool.IsFull());

    auto H = Pool.Alloc(7, 9);
    ASSERT_TRUE(H.IsValid());
    EXPECT_EQ(Pool.Num(), 1u);

    Widget* W = Pool.Resolve(H);
    ASSERT_NE(W, nullptr);
    EXPECT_EQ(W->X, 7);
    EXPECT_EQ(W->Y, 9);
    EXPECT_TRUE(Pool.IsValid(H));
}

TEST(TactixPool, FreeMakesHandleStaleAndBumpsGeneration)
{
    Tactix::FTactixPool<Widget, 4> Pool;
    auto H = Pool.Alloc(1, 2);
    ASSERT_TRUE(H.IsValid());

    const uint32_t GenBefore = H.GetGeneration();
    EXPECT_TRUE(Pool.Free(H));
    EXPECT_EQ(Pool.Num(), 0u);

    // Stale handle must NOT resolve, and freeing it twice must fail.
    EXPECT_EQ(Pool.Resolve(H), nullptr);
    EXPECT_FALSE(Pool.IsValid(H));
    EXPECT_FALSE(Pool.Free(H));

    // Re-alloc picks up the same slot but with a fresh generation.
    auto H2 = Pool.Alloc(11, 12);
    ASSERT_TRUE(H2.IsValid());
    EXPECT_EQ(H2.GetIndex(), H.GetIndex());
    EXPECT_GT(H2.GetGeneration(), GenBefore);
    EXPECT_EQ(Pool.Resolve(H), nullptr);      // original still stale
    EXPECT_NE(Pool.Resolve(H2), nullptr);     // new one lives
}

TEST(TactixPool, ExhaustionReturnsInvalidHandle)
{
    Tactix::FTactixPool<Widget, 2> Pool;
    auto A = Pool.Alloc(0, 0); ASSERT_TRUE(A.IsValid());
    auto B = Pool.Alloc(0, 0); ASSERT_TRUE(B.IsValid());
    EXPECT_TRUE(Pool.IsFull());

    auto C = Pool.Alloc(0, 0);
    EXPECT_FALSE(C.IsValid());

    // Free then re-alloc succeeds again.
    EXPECT_TRUE(Pool.Free(A));
    EXPECT_FALSE(Pool.IsFull());
    auto D = Pool.Alloc(0, 0);
    EXPECT_TRUE(D.IsValid());
}

TEST(TactixPool, FreeRejectsInvalidAndOutOfRangeHandles)
{
    Tactix::FTactixPool<Widget, 2> Pool;

    // Default-constructed handle is invalid.
    EXPECT_FALSE(Pool.Free(Tactix::FTactixHandle<Widget>{}));

    // A handle whose index is outside the pool must not crash.
    Tactix::FTactixHandle<Widget> Alien{9999, 1};
    EXPECT_FALSE(Pool.Free(Alien));
    EXPECT_EQ(Pool.Resolve(Alien), nullptr);
}

TEST(TactixPool, ForEachVisitsOnlyLiveSlots)
{
    Tactix::FTactixPool<Widget, 4> Pool;
    auto A = Pool.Alloc(1, 1);
    auto B = Pool.Alloc(2, 2);
    auto C = Pool.Alloc(3, 3);
    EXPECT_TRUE(Pool.Free(B));
    (void)A; (void)C;

    int Sum   = 0;
    int Count = 0;
    Pool.ForEach([&](auto, Widget& W) { Sum += W.X; ++Count; });

    EXPECT_EQ(Count, 2);  // only A and C live
    EXPECT_EQ(Sum,   4);  // 1 + 3
}

TEST(TactixPool, DestructorRunsForNonTriviallyDestructibleT)
{
    LifetimeCounter::Alive = 0;
    {
        Tactix::FTactixPool<LifetimeCounter, 4> Pool;
        auto A = Pool.Alloc(std::string("a"));
        auto B = Pool.Alloc(std::string("b"));
        (void)A; (void)B;
        EXPECT_EQ(LifetimeCounter::Alive, 2);

        EXPECT_TRUE(Pool.Free(A));
        EXPECT_EQ(LifetimeCounter::Alive, 1);
    } // Pool dtor frees B
    EXPECT_EQ(LifetimeCounter::Alive, 0);
}
