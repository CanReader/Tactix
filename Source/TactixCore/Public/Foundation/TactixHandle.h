// Copyright Sleak Software. All Rights Reserved.
//
// TactixHandle — type-safe generational handle template.
//
// Handles are the ONLY way to refer to a pooled object across frames in
// Tactix. Storing a raw T* is forbidden because the pool may reuse the slot,
// silently aliasing the pointer to a new (unrelated) object. Handles pack a
// 16-bit index and a 16-bit generation into a single uint32 so that:
//
//   • two handles pointing to different pool slots compare unequal;
//   • a handle pointing to a slot that was freed and re-allocated fails
//     validation because its stored generation lags the slot's current gen;
//   • a zero-initialized handle (packed == 0) is guaranteed to be "invalid"
//     and resolves to nullptr when queried against any pool.
//
// The `TagT` template parameter is purely a phantom type — it does not
// affect layout, but gives the type system enough information to prevent
// `FTactixHandle<FPlanNode>` from being assigned to `FTactixHandle<ITactixAgent>`.

#pragma once

#include "TactixApi.h"

#include <cstdint>

namespace Tactix
{
	template <typename TagT>
	struct FTactixHandle
	{
		using UnderlyingType = uint32_t;

		static constexpr uint32_t kIndexBits = 16;
		static constexpr uint32_t kGenBits   = 16;
		static constexpr uint32_t kIndexMask = (uint32_t{1} << kIndexBits) - 1;
		static constexpr uint32_t kGenMask   = (uint32_t{1} << kGenBits)   - 1;
		static constexpr uint32_t kMaxIndex  = kIndexMask;
		static constexpr uint32_t kMaxGen    = kGenMask;

		// Packed layout: [high 16 = generation][low 16 = index].
		// A handle with Packed == 0 is reserved as the invalid handle, which
		// means real entries must use Generation >= 1.
		uint32_t Packed{0};

		constexpr FTactixHandle() = default;

		constexpr FTactixHandle(uint32_t InIndex, uint32_t InGeneration)
			: Packed(((InGeneration & kGenMask) << kIndexBits) | (InIndex & kIndexMask))
		{
		}

		TACTIX_NODISCARD constexpr uint32_t GetIndex()      const { return Packed & kIndexMask; }
		TACTIX_NODISCARD constexpr uint32_t GetGeneration() const { return (Packed >> kIndexBits) & kGenMask; }
		TACTIX_NODISCARD constexpr bool     IsValid()       const { return Packed != 0; }

		constexpr bool operator==(FTactixHandle R) const { return Packed == R.Packed; }
		constexpr bool operator!=(FTactixHandle R) const { return Packed != R.Packed; }
		constexpr bool operator< (FTactixHandle R) const { return Packed <  R.Packed; }

		static constexpr FTactixHandle Invalid() { return FTactixHandle{}; }
	};

	static_assert(sizeof(FTactixHandle<int>) == 4, "FTactixHandle must be exactly 32 bits.");
}
