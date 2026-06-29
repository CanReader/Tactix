// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixHandle.h
 * @brief Type-safe generational handle: the only sanctioned way to reference a
 *        pooled object across frames.
 *
 * Storing a raw @c T* into a pooled object is a bug waiting to happen. The pool
 * reuses freed slots, so a pointer you cached last frame can quietly start
 * aliasing a completely unrelated object this frame, with no crash to tell you.
 * A handle dodges that by carrying a generation alongside the index and letting
 * the pool reject the lookup when the two disagree.
 *
 * Everything is packed into one 32-bit word, which buys three properties the
 * systems above rely on:
 *  - handles to different slots compare unequal, so they work as map keys;
 *  - a handle into a slot that was freed and re-allocated fails validation,
 *    because its stored generation now lags the slot's current generation;
 *  - a zero-initialised handle (`Packed == 0`) is the canonical invalid handle
 *    and resolves to @c nullptr against any pool, so default construction is
 *    automatically "empty".
 *
 * @tparam TagT A phantom tag, never instantiated. It has zero effect on layout
 *         but makes `FTactixHandle<FPlanNode>` a different type from
 *         `FTactixHandle<ITactixAgent>`, so the compiler refuses to mix them.
 */

#pragma once

#include "TactixApi.h"

#include <cstdint>

namespace Tactix
{
	template <typename TagT>
	struct FTactixHandle
	{
		/** @brief The single integer the whole handle packs into. */
		using UnderlyingType = uint32_t;

		static constexpr uint32_t kIndexBits = 16;                               ///< Bits reserved for the slot index.
		static constexpr uint32_t kGenBits   = 16;                               ///< Bits reserved for the generation.
		static constexpr uint32_t kIndexMask = (uint32_t{1} << kIndexBits) - 1;  ///< Low-word mask for the index.
		static constexpr uint32_t kGenMask   = (uint32_t{1} << kGenBits)   - 1;  ///< Mask for the generation field.
		static constexpr uint32_t kMaxIndex  = kIndexMask;                       ///< Largest representable slot index (65535).
		static constexpr uint32_t kMaxGen    = kGenMask;                         ///< Largest generation before it wraps.

		/**
		 * @brief Packed payload, layout `[high 16 = generation][low 16 = index]`.
		 *
		 * @c Packed == 0 is reserved to mean "invalid", which is why valid pool
		 * slots always start their generation at 1 rather than 0. Without that
		 * reservation a real handle to slot 0 / generation 0 would be
		 * indistinguishable from a default-constructed empty handle.
		 */
		uint32_t Packed{0};

		/** @brief Constructs the invalid handle (`Packed == 0`). */
		constexpr FTactixHandle() = default;

		/**
		 * @brief Packs an index and generation into a handle.
		 * @param InIndex      Slot index; only the low @ref kIndexBits are kept.
		 * @param InGeneration Slot generation; only the low @ref kGenBits are kept.
		 * @note Both values are masked rather than checked. The pool is expected
		 *       to have already bounded the index against its capacity.
		 */
		constexpr FTactixHandle(uint32_t InIndex, uint32_t InGeneration)
			: Packed(((InGeneration & kGenMask) << kIndexBits) | (InIndex & kIndexMask))
		{
		}

		/** @brief Unpacks the slot index. */
		TACTIX_NODISCARD constexpr uint32_t GetIndex()      const { return Packed & kIndexMask; }
		/** @brief Unpacks the generation stamped at allocation time. */
		TACTIX_NODISCARD constexpr uint32_t GetGeneration() const { return (Packed >> kIndexBits) & kGenMask; }
		/**
		 * @brief Cheap non-null test.
		 * @return True for any handle that isn't the zero sentinel.
		 * @warning This only says the handle isn't the empty value. It does not
		 *          prove the slot is still alive at the stored generation; only
		 *          @c FTactixPool::Resolve can answer that.
		 */
		TACTIX_NODISCARD constexpr bool     IsValid()       const { return Packed != 0; }

		constexpr bool operator==(FTactixHandle R) const { return Packed == R.Packed; }
		constexpr bool operator!=(FTactixHandle R) const { return Packed != R.Packed; }
		/** @brief Total order on the packed value, enough to key an ordered map. */
		constexpr bool operator< (FTactixHandle R) const { return Packed <  R.Packed; }

		/** @brief The canonical invalid handle. */
		static constexpr FTactixHandle Invalid() { return FTactixHandle{}; }
	};

	/// Handles must stay exactly 32 bits or the pool's packing math breaks.
	static_assert(sizeof(FTactixHandle<int>) == 4, "FTactixHandle must be exactly 32 bits.");
}
