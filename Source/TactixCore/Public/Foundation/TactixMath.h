// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixMath.h
 * @brief Small, engine-agnostic vector and scalar math used across Tactix.
 *
 * These types are deliberately not UE's @c FVector. Keeping a self-contained
 * @ref Tactix::FTactixVec3 is what lets TactixCore (and TactixSystems on top of
 * it) compile and unit-test with no Unreal dependency at all. The conversions
 * between @c FVector and these types live in TactixUE, where the engine is
 * already in scope; they have no place down here.
 *
 * Nothing fancy in the implementations, just the operations the AI systems
 * actually use: dot, cross, length, normalise, distance, and a handful of scalar
 * helpers.
 */

#pragma once

#include "TactixApi.h"

#include <cmath>
#include <cstdint>

namespace Tactix
{
	/** @brief Two-component float vector. Plain data, constexpr-friendly. */
	struct FTactixVec2
	{
		float X{0.0f};  ///< First component.
		float Y{0.0f};  ///< Second component.

		/** @brief Zero vector. */
		constexpr FTactixVec2() = default;
		/** @brief Component-wise constructor. */
		constexpr FTactixVec2(float InX, float InY) : X(InX), Y(InY) {}

		constexpr FTactixVec2 operator+(const FTactixVec2& R) const { return { X + R.X, Y + R.Y }; }
		constexpr FTactixVec2 operator-(const FTactixVec2& R) const { return { X - R.X, Y - R.Y }; }
		constexpr FTactixVec2 operator*(float S)             const { return { X * S,   Y * S   }; }
		constexpr FTactixVec2 operator/(float S)             const { return { X / S,   Y / S   }; }
		constexpr FTactixVec2 operator-()                    const { return { -X, -Y }; }

		constexpr bool operator==(const FTactixVec2& R) const { return X == R.X && Y == R.Y; }
		constexpr bool operator!=(const FTactixVec2& R) const { return !(*this == R); }

		/** @brief Dot product with @p R. */
		constexpr float Dot(const FTactixVec2& R) const { return X * R.X + Y * R.Y; }
		/** @brief Squared magnitude. Prefer this when you only need to compare lengths. */
		float LengthSquared() const { return X * X + Y * Y; }
		/** @brief Euclidean magnitude. */
		float Length() const { return std::sqrt(LengthSquared()); }
	};

	/** @brief Three-component float vector. Plain data, constexpr-friendly. */
	struct FTactixVec3
	{
		float X{0.0f};  ///< First component.
		float Y{0.0f};  ///< Second component.
		float Z{0.0f};  ///< Third component.

		/** @brief Zero vector. */
		constexpr FTactixVec3() = default;
		/** @brief Component-wise constructor. */
		constexpr FTactixVec3(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}

		constexpr FTactixVec3 operator+(const FTactixVec3& R) const { return { X + R.X, Y + R.Y, Z + R.Z }; }
		constexpr FTactixVec3 operator-(const FTactixVec3& R) const { return { X - R.X, Y - R.Y, Z - R.Z }; }
		constexpr FTactixVec3 operator*(float S)             const { return { X * S,   Y * S,   Z * S   }; }
		constexpr FTactixVec3 operator/(float S)             const { return { X / S,   Y / S,   Z / S   }; }
		constexpr FTactixVec3 operator-()                    const { return { -X, -Y, -Z }; }

		constexpr bool operator==(const FTactixVec3& R) const { return X == R.X && Y == R.Y && Z == R.Z; }
		constexpr bool operator!=(const FTactixVec3& R) const { return !(*this == R); }

		/** @brief Dot product with @p R. */
		constexpr float Dot(const FTactixVec3& R) const { return X * R.X + Y * R.Y + Z * R.Z; }
		/** @brief Cross product `this x R`, following the right-hand rule. */
		constexpr FTactixVec3 Cross(const FTactixVec3& R) const
		{
			return { Y * R.Z - Z * R.Y, Z * R.X - X * R.Z, X * R.Y - Y * R.X };
		}

		/** @brief Squared magnitude. Prefer this when only comparing lengths. */
		float LengthSquared() const { return X * X + Y * Y + Z * Z; }
		/** @brief Euclidean magnitude. */
		float Length() const { return std::sqrt(LengthSquared()); }

		/**
		 * @brief Returns a unit-length copy, or the zero vector if too short.
		 * @param Epsilon Squared-length floor. At or below it the vector is treated
		 *        as degenerate and the zero vector is returned instead of dividing
		 *        by a near-zero length.
		 */
		FTactixVec3 Normalized(float Epsilon = 1e-8f) const
		{
			const float L2 = LengthSquared();
			if (L2 <= Epsilon) return {0.0f, 0.0f, 0.0f};
			const float Inv = 1.0f / std::sqrt(L2);
			return { X * Inv, Y * Inv, Z * Inv };
		}
	};

	/**
	 * @brief Clamps @p V into the inclusive range `[Lo, Hi]`.
	 * @pre @p Lo must not exceed @p Hi; the function doesn't reorder them.
	 */
	template <typename T>
	constexpr T Clamp(T V, T Lo, T Hi) { return V < Lo ? Lo : (V > Hi ? Hi : V); }

	/** @brief Smaller of @p A and @p B. */
	template <typename T>
	constexpr T Min(T A, T B) { return A < B ? A : B; }

	/** @brief Larger of @p A and @p B. */
	template <typename T>
	constexpr T Max(T A, T B) { return A < B ? B : A; }

	/**
	 * @brief Linear interpolation from @p A to @p B by @p T.
	 * @param A Start value, returned when @p T is 0.
	 * @param B End value, returned when @p T is 1.
	 * @param T Blend factor. Not clamped, so values outside [0, 1] extrapolate.
	 */
	constexpr float Lerp(float A, float B, float T) { return A + (B - A) * T; }

	/** @brief Distance between two 3D points. */
	inline float Distance(const FTactixVec3& A, const FTactixVec3& B) { return (A - B).Length(); }
	/** @brief Squared distance between two 3D points; cheaper, comparison-only. */
	inline float DistanceSquared(const FTactixVec3& A, const FTactixVec3& B) { return (A - B).LengthSquared(); }
	/** @brief Distance between two 2D points. */
	inline float Distance2D(const FTactixVec2& A, const FTactixVec2& B) { return (A - B).Length(); }

	/** @brief Clamps @p X to [0, 1]. The common case for utility scores. */
	TACTIX_FORCEINLINE constexpr float Saturate(float X) { return X < 0.0f ? 0.0f : (X > 1.0f ? 1.0f : X); }
}
