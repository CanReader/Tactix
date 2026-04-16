// Copyright Sleak Software. All Rights Reserved.
//
// Tests for the engine-agnostic math primitives in TactixMath.h. These mirror
// (a small slice of) what FVector gives us but without any UE dependency, so
// the suite below leans on the published semantics only.

#include "Foundation/TactixMath.h"

#include <gtest/gtest.h>

#include <cmath>

namespace
{
    constexpr float kEps = 1e-5f;
}

TEST(TactixMath, Vec2Arithmetic)
{
    Tactix::FTactixVec2 A{1.0f, 2.0f};
    Tactix::FTactixVec2 B{3.0f, 4.0f};

    EXPECT_EQ(A + B, Tactix::FTactixVec2(4.0f, 6.0f));
    EXPECT_EQ(B - A, Tactix::FTactixVec2(2.0f, 2.0f));
    EXPECT_EQ(A * 2.0f, Tactix::FTactixVec2(2.0f, 4.0f));
    EXPECT_FLOAT_EQ(A.Dot(B), 11.0f);
}

TEST(TactixMath, Vec3ArithmeticDotCross)
{
    Tactix::FTactixVec3 X{1.0f, 0.0f, 0.0f};
    Tactix::FTactixVec3 Y{0.0f, 1.0f, 0.0f};
    Tactix::FTactixVec3 Z{0.0f, 0.0f, 1.0f};

    EXPECT_EQ(X.Cross(Y), Z);
    EXPECT_EQ(Y.Cross(X), -Z);
    EXPECT_FLOAT_EQ(X.Dot(Y), 0.0f);
    EXPECT_FLOAT_EQ(X.Dot(X), 1.0f);
}

TEST(TactixMath, Vec3Length)
{
    Tactix::FTactixVec3 V{3.0f, 4.0f, 0.0f};
    EXPECT_FLOAT_EQ(V.LengthSquared(), 25.0f);
    EXPECT_FLOAT_EQ(V.Length(), 5.0f);
}

TEST(TactixMath, Vec3NormalizedZeroVectorReturnsZero)
{
    Tactix::FTactixVec3 Zero{};
    Tactix::FTactixVec3 N = Zero.Normalized();
    EXPECT_EQ(N, (Tactix::FTactixVec3{0.0f, 0.0f, 0.0f}));
}

TEST(TactixMath, Vec3NormalizedUnitLength)
{
    Tactix::FTactixVec3 V{0.0f, 3.0f, 4.0f};
    Tactix::FTactixVec3 N = V.Normalized();
    EXPECT_NEAR(N.Length(), 1.0f, kEps);
}

TEST(TactixMath, ClampMinMax)
{
    EXPECT_EQ(Tactix::Clamp(5, 0, 10), 5);
    EXPECT_EQ(Tactix::Clamp(-1, 0, 10), 0);
    EXPECT_EQ(Tactix::Clamp(11, 0, 10), 10);

    EXPECT_EQ(Tactix::Min(1, 2), 1);
    EXPECT_EQ(Tactix::Max(1, 2), 2);
}

TEST(TactixMath, SaturateClampsZeroToOne)
{
    EXPECT_FLOAT_EQ(Tactix::Saturate(-0.5f), 0.0f);
    EXPECT_FLOAT_EQ(Tactix::Saturate( 0.5f), 0.5f);
    EXPECT_FLOAT_EQ(Tactix::Saturate( 1.5f), 1.0f);
}

TEST(TactixMath, LerpEndpointsAndMidpoint)
{
    EXPECT_FLOAT_EQ(Tactix::Lerp(0.0f, 10.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(Tactix::Lerp(0.0f, 10.0f, 1.0f), 10.0f);
    EXPECT_FLOAT_EQ(Tactix::Lerp(0.0f, 10.0f, 0.5f), 5.0f);
}

TEST(TactixMath, DistanceHelpers)
{
    Tactix::FTactixVec3 A{0.0f, 0.0f, 0.0f};
    Tactix::FTactixVec3 B{1.0f, 2.0f, 2.0f};
    EXPECT_NEAR(Tactix::Distance(A, B), 3.0f, kEps);
    EXPECT_NEAR(Tactix::DistanceSquared(A, B), 9.0f, kEps);

    Tactix::FTactixVec2 P{0.0f, 0.0f};
    Tactix::FTactixVec2 Q{3.0f, 4.0f};
    EXPECT_NEAR(Tactix::Distance2D(P, Q), 5.0f, kEps);
}
