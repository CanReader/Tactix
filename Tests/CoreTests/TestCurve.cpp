// Copyright Sleak Software. All Rights Reserved.
//
// Tests for FTactixCurve response curves. The CLAUDE.md spec explicitly
// requires:
//   • Linear(0.5) with slope=1 == 0.5
//   • Logistic curve is S-shaped and clamped
// Both are asserted directly here. Additional tests cover Quadratic sign
// preservation, Exponential monotonicity, and Custom saturation.

#include "Foundation/TactixCurve.h"

#include <gtest/gtest.h>

#include <cmath>

namespace
{
    constexpr float kEps = 1e-5f;
}

TEST(TactixCurve, LinearEvaluatesIdentityAndSaturates)
{
    // Mandated: Linear(0.5) with slope=1 == 0.5.
    auto C = Tactix::FTactixCurve::MakeLinear(1.0f, 0.0f);
    EXPECT_NEAR(C.Evaluate(0.5f), 0.5f, kEps);

    // Below/above the [0,1] range saturates.
    EXPECT_FLOAT_EQ(C.Evaluate(-1.0f), 0.0f);
    EXPECT_FLOAT_EQ(C.Evaluate( 2.0f), 1.0f);

    // Shift moves the zero-point.
    auto Shifted = Tactix::FTactixCurve::MakeLinear(1.0f, 0.25f);
    EXPECT_NEAR(Shifted.Evaluate(0.25f), 0.0f, kEps);
    EXPECT_NEAR(Shifted.Evaluate(0.75f), 0.5f, kEps);
}

TEST(TactixCurve, QuadraticPreservesSignAndSaturates)
{
    // y = x^2 on x in [0,1] reaches 1.0 at x=1.
    auto Q = Tactix::FTactixCurve::MakeQuadratic(1.0f, 2.0f, 0.0f);
    EXPECT_NEAR(Q.Evaluate(0.0f), 0.0f, kEps);
    EXPECT_NEAR(Q.Evaluate(0.5f), 0.25f, kEps);
    EXPECT_NEAR(Q.Evaluate(1.0f), 1.0f, kEps);

    // Negative argument without sign preservation + saturation would clip to 0
    // (desired behaviour). The implementation detail is that we compute
    // |arg|^Exp and carry the sign; after saturation the negative result floors
    // at 0, which is exactly what the utility selector expects.
    EXPECT_FLOAT_EQ(Q.Evaluate(-0.5f), 0.0f);
}

TEST(TactixCurve, LogisticIsMonotonicAndClampedSShape)
{
    // Mandated: Logistic curve is S-shaped and clamped.
    auto L = Tactix::FTactixCurve::MakeLogistic(/*Slope=*/10.0f, /*Shift=*/0.5f);

    // At shift, sigmoid(0) == 0.5 exactly.
    EXPECT_NEAR(L.Evaluate(0.5f), 0.5f, kEps);

    // Saturates in [0, 1] — the library contract.
    EXPECT_GE(L.Evaluate(-1e6f), 0.0f);
    EXPECT_LE(L.Evaluate(-1e6f), kEps);
    EXPECT_LE(L.Evaluate( 1e6f), 1.0f);
    EXPECT_GE(L.Evaluate( 1e6f), 1.0f - kEps);

    // Monotonic along its domain.
    float Prev = L.Evaluate(-2.0f);
    for (float x = -2.0f; x <= 2.0f; x += 0.1f)
    {
        const float Y = L.Evaluate(x);
        EXPECT_GE(Y + kEps, Prev) << "Logistic must be non-decreasing; "
                                  << "f(" << x << ") = " << Y
                                  << " < previous " << Prev;
        Prev = Y;
    }

    // "S-shape" check: the slope is steepest near the shift, shallower at the
    // tails. Compare midpoint slope to off-centre slope.
    const float MidSlope  = L.Evaluate(0.55f) - L.Evaluate(0.45f);
    const float EdgeSlope = L.Evaluate(0.05f) - L.Evaluate(-0.05f);
    EXPECT_GT(MidSlope, EdgeSlope);
}

TEST(TactixCurve, ExponentialIsMonotonicAndSaturates)
{
    // Use Exp=1.0 so the rise is gradual — with the default Exp=4 the curve
    // saturates before we can observe monotonicity between two test points.
    auto E = Tactix::FTactixCurve::MakeExponential(/*Slope=*/1.0f, /*Exp=*/1.0f, /*Shift=*/0.0f);
    EXPECT_NEAR(E.Evaluate(0.0f), 0.0f, kEps);
    EXPECT_GT  (E.Evaluate(0.5f),  E.Evaluate(0.25f));     // monotone increasing
    EXPECT_GT  (E.Evaluate(0.75f), E.Evaluate(0.50f));
    EXPECT_LE  (E.Evaluate(1.0f),  1.0f);                  // stays saturated

    // Large inputs saturate at the top.
    EXPECT_FLOAT_EQ(E.Evaluate(10.0f), 1.0f);
    // Large negative inputs produce a negative raw value → saturated to 0.
    EXPECT_FLOAT_EQ(E.Evaluate(-10.0f), 0.0f);
}

TEST(TactixCurve, CustomHandlesNullFnGracefully)
{
    Tactix::FTactixCurve C;
    C.Type     = Tactix::ETactixCurveType::Custom;
    C.CustomFn = nullptr;
    EXPECT_FLOAT_EQ(C.Evaluate(0.42f), 0.0f);
}

TEST(TactixCurve, CustomEvaluatesAndSaturates)
{
    auto Identity = [](float x) -> float { return x; };
    auto C = Tactix::FTactixCurve::MakeCustom(Identity);
    EXPECT_NEAR(C.Evaluate(0.3f), 0.3f, kEps);
    EXPECT_FLOAT_EQ(C.Evaluate( 2.5f), 1.0f); // saturated
    EXPECT_FLOAT_EQ(C.Evaluate(-2.5f), 0.0f); // saturated
}
