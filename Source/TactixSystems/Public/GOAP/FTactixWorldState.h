// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixWorldState.h
 * @brief Symbolic world state for the GOAP planner: 64 boolean facts plus a
 *        "defined" mask.
 *
 * The planner reasons over symbolic facts ("HasAmmo", "InCover", "EnemyVisible"),
 * each one a bit addressed by index. Two 64-bit words carry everything: @c Values
 * holds the truth of each fact and @c Mask records which facts this state even has
 * an opinion about. A bit that's unset in @c Mask is "don't care", which is how a
 * goal can require just two facts while ignoring the other 62.
 *
 * This is intentionally booleans only. Float or scalar predicates (distances,
 * thresholds) don't belong here; they're handled in @ref Tactix::ITactixGOAPAction "ITactixGOAPAction" via
 * the agent context, leaving the planner's graph small and hashable.
 */

#pragma once

#include "TactixApi.h"

#include <cstdint>

namespace Tactix
{
	/**
	 * @brief A set of up to 64 boolean facts with per-fact "defined" tracking.
	 *
	 * Used for three roles in GOAP: the current state, an action's preconditions
	 * (which facts must hold), and an action's effects (which facts it sets). The
	 * mask semantics make a single type serve all three.
	 */
	struct FTactixWorldState
	{
		uint64_t Values{0};  ///< Bit i is the truth of fact i (only meaningful where @c Mask bit i is set).
		uint64_t Mask{0};    ///< Bit i set means "fact i is defined here"; clear means "don't care".

		/** @brief Number of addressable facts (the bit width). */
		static constexpr uint32_t kFactCount = 64;

		/**
		 * @brief Defines fact @p Index and sets its truth.
		 * @param Index Fact index; taken modulo 64.
		 * @param Value Truth value to store.
		 */
		constexpr void Set(uint32_t Index, bool Value)
		{
			const uint64_t Bit = uint64_t{1} << (Index & 63);
			Mask |= Bit;
			Values = Value ? (Values | Bit) : (Values & ~Bit);
		}

		/**
		 * @brief Marks fact @p Index as undefined ("don't care") again.
		 * @param Index Fact index; taken modulo 64.
		 */
		constexpr void Unset(uint32_t Index)
		{
			const uint64_t Bit = uint64_t{1} << (Index & 63);
			Mask   &= ~Bit;
			Values &= ~Bit;
		}

		/** @brief Whether fact @p Index is defined in this state. */
		constexpr bool IsDefined(uint32_t Index) const
		{
			return (Mask & (uint64_t{1} << (Index & 63))) != 0;
		}

		/**
		 * @brief Truth of fact @p Index.
		 * @return The stored bit. Meaningless unless @ref IsDefined is true for it.
		 */
		constexpr bool Get(uint32_t Index) const
		{
			return (Values & (uint64_t{1} << (Index & 63))) != 0;
		}

		/**
		 * @brief Whether this state meets a requirement.
		 * @param Required The facts that must hold (its mask says which matter).
		 * @return True when every fact @p Required defines has the same truth here.
		 *         Facts @p Required leaves undefined are ignored.
		 */
		constexpr bool Satisfies(const FTactixWorldState& Required) const
		{
			return (Values & Required.Mask) == (Required.Values & Required.Mask);
		}

		/**
		 * @brief Applies an effect by overlaying its defined facts onto this state.
		 * @param Delta The effect; only the facts it defines are written.
		 */
		constexpr void Apply(const FTactixWorldState& Delta)
		{
			Values = (Values & ~Delta.Mask) | (Delta.Values & Delta.Mask);
			Mask  |= Delta.Mask;
		}

		/**
		 * @brief A* heuristic: how many of @p Target's facts are currently wrong.
		 * @param Target The goal state.
		 * @return Count of @p Target's defined facts that differ here. This is
		 *         admissible (never overestimates), since each wrong fact needs at
		 *         least one action to flip, so A* stays optimal.
		 */
		constexpr uint32_t Distance(const FTactixWorldState& Target) const
		{
			return PopCount64((Values ^ Target.Values) & Target.Mask);
		}

		constexpr bool operator==(const FTactixWorldState& R) const
		{
			return Values == R.Values && Mask == R.Mask;
		}

		constexpr bool operator!=(const FTactixWorldState& R) const { return !(*this == R); }

		/** @brief 64-bit FNV-1a hash of (Values, Mask), used to key the closed set. */
		constexpr uint64_t Hash() const
		{
			uint64_t H = 1469598103934665603ull;
			H ^= Values; H *= 1099511628211ull;
			H ^= Mask;   H *= 1099511628211ull;
			return H;
		}

	private:
		/** @brief Branch-free 64-bit population count (SWAR). */
		static constexpr uint32_t PopCount64(uint64_t V)
		{
			V = V - ((V >> 1) & 0x5555555555555555ull);
			V = (V & 0x3333333333333333ull) + ((V >> 2) & 0x3333333333333333ull);
			V = (V + (V >> 4)) & 0x0f0f0f0f0f0f0f0full;
			return static_cast<uint32_t>((V * 0x0101010101010101ull) >> 56);
		}
	};
}
