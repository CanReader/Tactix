// Copyright Sleak Software. All Rights Reserved.
//
// Engine-agnostic math primitives used throughout Tactix. These are
// deliberately independent of UE's FVector so TactixCore can be compiled and
// unit-tested without any Unreal dependency. Adapters between FVector and
// FTactixVec3 live in TactixUE, not here.

#pragma once

#include "TactixApi.h"

#include <cmath>
#include <cstdint>

namespace Tactix
{
	// ---- 2D vector ---------------------------------------------------------

	struct FTactixVec2
	{
		float X{0.0f};
		float Y{0.0f};

		constexpr FTactixVec2() = default;
		constexpr FTactixVec2(float InX, float InY) : X(InX), Y(InY) {}

		constexpr FTactixVec2 operator+(const FTactixVec2& R) const { return { X + R.X, Y + R.Y }; }
		constexpr FTactixVec2 operator-(const FTactixVec2& R) const { return { X - R.X, Y - R.Y }; }
		constexpr FTactixVec2 operator*(float S)             const { return { X * S,   Y * S   }; }
		constexpr FTactixVec2 operator/(float S)             const { return { X / S,   Y / S   }; }
		constexpr FTactixVec2 operator-()                    const { return { -X, -Y }; }

		constexpr bool operator==(const FTactixVec2& R) const { return X == R.X && Y == R.Y; }
		constexpr bool operator!=(const FTactixVec2& R) const { return !(*this == R); }

		constexpr float Dot(const FTactixVec2& R) const { return X * R.X + Y * R.Y; }
		float LengthSquared() const { return X * X + Y * Y; }
		float Length() const { return std::sqrt(LengthSquared()); }
	};

	// ---- 3D vector ---------------------------------------------------------

	struct FTactixVec3
	{
		float X{0.0f};
		float Y{0.0f};
		float Z{0.0f};

		constexpr FTactixVec3() = default;
		constexpr FTactixVec3(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}

		constexpr FTactixVec3 operator+(const FTactixVec3& R) const { return { X + R.X, Y + R.Y, Z + R.Z }; }
		constexpr FTactixVec3 operator-(const FTactixVec3& R) const { return { X - R.X, Y - R.Y, Z - R.Z }; }
		constexpr FTactixVec3 operator*(float S)             const { return { X * S,   Y * S,   Z * S   }; }
		constexpr FTactixVec3 operator/(float S)             const { return { X / S,   Y / S,   Z / S   }; }
		constexpr FTactixVec3 operator-()                    const { return { -X, -Y, -Z }; }

		constexpr bool operator==(const FTactixVec3& R) const { return X == R.X && Y == R.Y && Z == R.Z; }
		constexpr bool operator!=(const FTactixVec3& R) const { return !(*this == R); }

		constexpr float Dot(const FTactixVec3& R) const { return X * R.X + Y * R.Y + Z * R.Z; }
		constexpr FTactixVec3 Cross(const FTactixVec3& R) const
		{
			return { Y * R.Z - Z * R.Y, Z * R.X - X * R.Z, X * R.Y - Y * R.X };
		}

		float LengthSquared() const { return X * X + Y * Y + Z * Z; }
		float Length() const { return std::sqrt(LengthSquared()); }

		FTactixVec3 Normalized(float Epsilon = 1e-8f) const
		{
			const float L2 = LengthSquared();
			if (L2 <= Epsilon) return {0.0f, 0.0f, 0.0f};
			const float Inv = 1.0f / std::sqrt(L2);
			return { X * Inv, Y * Inv, Z * Inv };
		}
	};

	// ---- Scalar helpers ----------------------------------------------------

	template <typename T>
	constexpr T Clamp(T V, T Lo, T Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }

	template <typename T>
	constexpr T Min(T A, T B) { return A < B ? A : B; }

	template <typename T>
	constexpr T Max(T A, T B) { return A < B ? B : A; }

	constexpr float Lerp(float A, float B, float T) { return A + (B - A) * T; }

	inline float Distance(const FTactixVec3& A, const FTactixVec3& B) { return (A - B).Length(); }
	inline float DistanceSquared(const FTactixVec3& A, const FTactixVec3& B) { return (A - B).LengthSquared(); }
	inline float Distance2D(const FTactixVec2& A, const FTactixVec2& B) { return (A - B).Length(); }

	// Saturate = Clamp to [0, 1] — handy for Utility AI curves.
	TACTIX_FORCEINLINE constexpr float Saturate(float X) { return X < 0.0f ? 0.0f : (X > 1.0f ? 1.0f : X); }
}
