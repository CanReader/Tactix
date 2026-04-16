// Copyright Sleak Software. All Rights Reserved.
//
// Tests for FTactixHandle — the generational handle template that is the sole
// allowed cross-frame reference to pooled objects. These tests verify layout,
// construction, equality, and the reserved "packed == 0 means invalid" rule.

#include "Foundation/TactixHandle.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

namespace
{
    struct AgentTag {};
    struct PlanTag  {};
}

using AgentHandle = Tactix::FTactixHandle<AgentTag>;
using PlanHandle  = Tactix::FTactixHandle<PlanTag>;

TEST(TactixHandle, DefaultConstructsInvalid)
{
    AgentHandle H;
    EXPECT_FALSE(H.IsValid());
    EXPECT_EQ(H.Packed, 0u);
    EXPECT_EQ(H, AgentHandle::Invalid());
}

TEST(TactixHandle, ConstructPacksIndexAndGeneration)
{
    AgentHandle H{0x0ABC, 0x0007};
    EXPECT_TRUE(H.IsValid());
    EXPECT_EQ(H.GetIndex(),      0x0ABCu);
    EXPECT_EQ(H.GetGeneration(), 0x0007u);
    // Generation in the high 16 bits, index in the low 16.
    EXPECT_EQ(H.Packed, (0x0007u << 16) | 0x0ABCu);
}

TEST(TactixHandle, MasksOverflowInputs)
{
    // Passing bits that don't fit must be silently masked, not smear into the
    // other field.
    AgentHandle H{0xFFFFFFFFu, 0xFFFFFFFFu};
    EXPECT_EQ(H.GetIndex(),      AgentHandle::kIndexMask);
    EXPECT_EQ(H.GetGeneration(), AgentHandle::kGenMask);
}

TEST(TactixHandle, EqualityAndOrdering)
{
    AgentHandle A{5, 2};
    AgentHandle B{5, 2};
    AgentHandle C{6, 2};

    EXPECT_EQ(A, B);
    EXPECT_NE(A, C);
    EXPECT_TRUE(A < C);
}

TEST(TactixHandle, DifferentTagsAreUnrelatedTypes)
{
    // FTactixHandle<A> and FTactixHandle<B> must not implicit-convert — the
    // whole point of the phantom tag is to catch this at compile time.
    static_assert(!std::is_convertible_v<AgentHandle, PlanHandle>,
                  "Different-tag handles must NOT be implicitly convertible.");
    static_assert(!std::is_convertible_v<PlanHandle, AgentHandle>,
                  "Different-tag handles must NOT be implicitly convertible.");
    SUCCEED();
}

TEST(TactixHandle, LayoutIsExactly32Bits)
{
    static_assert(sizeof(AgentHandle) == 4, "FTactixHandle must be 32 bits.");
    static_assert(std::is_trivially_copyable_v<AgentHandle>,
                  "Handles must be trivially copyable to stay in pool/arena memory.");
    SUCCEED();
}

TEST(TactixHandle, MaxIndexAndGenerationFit16Bits)
{
    EXPECT_EQ(AgentHandle::kMaxIndex, 0xFFFFu);
    EXPECT_EQ(AgentHandle::kMaxGen,   0xFFFFu);
}
