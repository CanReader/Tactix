// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"

#include <cstdint>

namespace Tactix
{
	// 64 boolean facts addressed by index. Mask marks which bits are defined;
	// undefined bits are "don't care" during matching. Float predicates live
	// outside this struct — the planner only reasons about booleans.
	struct FTactixWorldState
	{
		uint64_t Values{0};
		uint64_t Mask{0};

		static constexpr uint32_t kFactCount = 64;

		constexpr void Set(uint32_t Index, bool Value)
		{
			const uint64_t Bit = uint64_t{1} << (Index & 63);
			Mask |= Bit;
			Values = Value ? (Values | Bit) : (Values & ~Bit);
		}

		constexpr void Unset(uint32_t Index)
		{
			const uint64_t Bit = uint64_t{1} << (Index & 63);
			Mask   &= ~Bit;
			Values &= ~Bit;
		}

		constexpr bool IsDefined(uint32_t Index) const
		{
			return (Mask & (uint64_t{1} << (Index & 63))) != 0;
		}

		constexpr bool Get(uint32_t Index) const
		{
			return (Values & (uint64_t{1} << (Index & 63))) != 0;
		}

		// Every defined bit in Required must equal ours.
		constexpr bool Satisfies(const FTactixWorldState& Required) const
		{
			return (Values & Required.Mask) == (Required.Values & Required.Mask);
		}

		// Overlay Delta's defined bits onto this state (action effects).
		constexpr void Apply(const FTactixWorldState& Delta)
		{
			Values = (Values & ~Delta.Mask) | (Delta.Values & Delta.Mask);
			Mask  |= Delta.Mask;
		}

		// Popcount of mismatched defined bits against Target. Admissible
		// heuristic for A*: each off fact takes at least one action to flip.
		constexpr uint32_t Distance(const FTactixWorldState& Target) const
		{
			return PopCount64((Values ^ Target.Values) & Target.Mask);
		}

		constexpr bool operator==(const FTactixWorldState& R) const
		{
			return Values == R.Values && Mask == R.Mask;
		}

		constexpr bool operator!=(const FTactixWorldState& R) const { return !(*this == R); }

		constexpr uint64_t Hash() const
		{
			uint64_t H = 1469598103934665603ull;
			H ^= Values; H *= 1099511628211ull;
			H ^= Mask;   H *= 1099511628211ull;
			return H;
		}

	private:
		static constexpr uint32_t PopCount64(uint64_t V)
		{
			V = V - ((V >> 1) & 0x5555555555555555ull);
			V = (V & 0x3333333333333333ull) + ((V >> 2) & 0x3333333333333333ull);
			V = (V + (V >> 4)) & 0x0f0f0f0f0f0f0f0full;
			return static_cast<uint32_t>((V * 0x0101010101010101ull) >> 56);
		}
	};
}
