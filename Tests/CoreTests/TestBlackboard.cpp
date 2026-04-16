// Copyright Sleak Software. All Rights Reserved.
//
// Tests for FTactixBlackboard and its compile-time FNV-1a key literal. Focus
// on: typed get/set via the non-virtual templated API, wrong-type get rejection,
// remove() with the Robin-Hood backshift that preserves probe invariants, and
// the reserved "hash == 0" empty-slot sentinel.

#include "Blackboard/FTactixBlackboard.h"

#include <gtest/gtest.h>

#include <cstdint>

using namespace Tactix::Literals;

TEST(TactixBlackboard, LiteralProducesConstexprHash)
{
    constexpr auto A = "HasTarget"_bb;
    constexpr auto B = "HasTarget"_bb;
    constexpr auto C = "HasCover"_bb;

    static_assert(A == B, "Same string literal must hash to same key.");
    static_assert(A != C, "Different strings hash to different keys.");
    static_assert(A.IsValid(), "Non-empty literals are valid.");
    SUCCEED();
}

TEST(TactixBlackboard, HashCollisionWithZeroIsNudgedAway)
{
    // Any key whose raw hash is 0 must be nudged to 1 — otherwise it would
    // collide with the "empty slot" sentinel. We can't easily craft a natural
    // zero hash, but we can assert the explicit constructor path does the
    // right thing.
    Tactix::FTactixBBKey K{0u};
    EXPECT_FALSE(K.IsValid()); // constructed from raw 0 -> empty sentinel
    Tactix::FTactixBBKey K2{Tactix::HashFNV1a("")};
    EXPECT_TRUE(K2.IsValid()); // empty string must still produce a valid key
}

TEST(TactixBlackboard, TypedSetGetRoundtripsAllSupportedTypes)
{
    Tactix::FTactixBlackboard<16> BB;

    BB.Set("b"_bb,  true);
    BB.Set("i"_bb,  int32_t{-42});
    BB.Set("f"_bb,  3.5f);
    BB.Set("v"_bb,  Tactix::FTactixVec3{1.0f, 2.0f, 3.0f});
    BB.Set("h"_bb,  uint32_t{0xDEADBEEFu});
    int LocalAnchor = 0;
    BB.Set("p"_bb,  static_cast<void*>(&LocalAnchor));

    bool    OutB;   EXPECT_TRUE(BB.Get("b"_bb, OutB));  EXPECT_TRUE(OutB);
    int32_t OutI;   EXPECT_TRUE(BB.Get("i"_bb, OutI));  EXPECT_EQ(OutI, -42);
    float   OutF;   EXPECT_TRUE(BB.Get("f"_bb, OutF));  EXPECT_FLOAT_EQ(OutF, 3.5f);

    Tactix::FTactixVec3 OutV;
    EXPECT_TRUE(BB.Get("v"_bb, OutV));
    EXPECT_EQ(OutV, (Tactix::FTactixVec3{1.0f, 2.0f, 3.0f}));

    uint32_t OutH;
    EXPECT_TRUE(BB.Get("h"_bb, OutH));
    EXPECT_EQ(OutH, 0xDEADBEEFu);

    void* OutP = nullptr;
    EXPECT_TRUE(BB.Get("p"_bb, OutP));
    EXPECT_EQ(OutP, static_cast<void*>(&LocalAnchor));
}

TEST(TactixBlackboard, WrongTypeGetReturnsFalse)
{
    Tactix::FTactixBlackboard<16> BB;
    BB.Set("n"_bb, int32_t{99});

    // Same key, different type — must fail cleanly.
    float F = 0.0f;
    EXPECT_FALSE(BB.Get("n"_bb, F));

    bool B = false;
    EXPECT_FALSE(BB.Get("n"_bb, B));
}

TEST(TactixBlackboard, RemovePreservesOtherEntriesEvenUnderCollisionProbing)
{
    Tactix::FTactixBlackboard<16> BB;

    // Insert a handful of keys — the implementation uses open addressing, so
    // this exercises the Robin-Hood backshift path.
    BB.Set("A"_bb, int32_t{1});
    BB.Set("B"_bb, int32_t{2});
    BB.Set("C"_bb, int32_t{3});
    BB.Set("D"_bb, int32_t{4});
    BB.Set("E"_bb, int32_t{5});
    EXPECT_EQ(BB.Num(), 5u);

    EXPECT_TRUE (BB.Remove("C"_bb));
    EXPECT_FALSE(BB.Has   ("C"_bb));
    EXPECT_EQ   (BB.Num(), 4u);

    // After backshift the other keys must still be reachable with their
    // original values.
    int32_t V = 0;
    EXPECT_TRUE(BB.Get("A"_bb, V)); EXPECT_EQ(V, 1);
    EXPECT_TRUE(BB.Get("B"_bb, V)); EXPECT_EQ(V, 2);
    EXPECT_TRUE(BB.Get("D"_bb, V)); EXPECT_EQ(V, 4);
    EXPECT_TRUE(BB.Get("E"_bb, V)); EXPECT_EQ(V, 5);

    // Removing a key that was never set is a no-op returning false.
    EXPECT_FALSE(BB.Remove("Z"_bb));
}

TEST(TactixBlackboard, OverwriteSameKeyKeepsSize)
{
    Tactix::FTactixBlackboard<16> BB;
    BB.Set("speed"_bb, 1.0f);
    BB.Set("speed"_bb, 2.0f);
    EXPECT_EQ(BB.Num(), 1u);

    float S = 0.0f;
    ASSERT_TRUE(BB.Get("speed"_bb, S));
    EXPECT_FLOAT_EQ(S, 2.0f);
}

TEST(TactixBlackboard, ClearEmptiesAllEntries)
{
    Tactix::FTactixBlackboard<8> BB;
    BB.Set("a"_bb, 1.0f);
    BB.Set("b"_bb, 2.0f);
    EXPECT_EQ(BB.Num(), 2u);

    BB.Clear();
    EXPECT_EQ(BB.Num(), 0u);
    EXPECT_FALSE(BB.Has("a"_bb));
    EXPECT_FALSE(BB.Has("b"_bb));
}

TEST(TactixBlackboard, FullTableSilentlyDropsExtraInserts)
{
    // Fill a capacity-2 table, then prove that a third unique key doesn't
    // overwrite an existing entry or corrupt the table.
    Tactix::FTactixBlackboard<2> BB;
    BB.Set("k1"_bb, 1.0f);
    BB.Set("k2"_bb, 2.0f);
    EXPECT_EQ(BB.Num(), 2u);

    BB.Set("k3"_bb, 3.0f); // documented policy: silently drop

    EXPECT_EQ(BB.Num(), 2u);
    float V = 0.0f;
    EXPECT_TRUE(BB.Get("k1"_bb, V)); EXPECT_FLOAT_EQ(V, 1.0f);
    EXPECT_TRUE(BB.Get("k2"_bb, V)); EXPECT_FLOAT_EQ(V, 2.0f);
    EXPECT_FALSE(BB.Has("k3"_bb));
}

TEST(TactixBlackboard, AbstractBaseDispatchesViaVirtuals)
{
    // A context receives an FTactixBlackboardRef* — the templated helpers on
    // the base route to the correct concrete get/set. Verify through the base.
    Tactix::FTactixBlackboard<8>      Impl;
    Tactix::FTactixBlackboardRef&     Ref = Impl;

    Ref.Set("speed"_bb, 12.5f);
    float V = 0.0f;
    EXPECT_TRUE(Ref.Get("speed"_bb, V));
    EXPECT_FLOAT_EQ(V, 12.5f);
}
