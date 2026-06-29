// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixPool.h
 * @brief Fixed-capacity pool allocator with per-slot generation tracking.
 *
 * The pool reserves storage for @p Capacity objects at construction and never
 * touches the heap again. Alloc and Free are both O(1): a free list threaded
 * through the unused slots' @c NextFree fields acts as a stack of available
 * indices, so allocation is a pop and deallocation is a push. That keeps it
 * inside the project's "no @c new on the hot path" rule.
 *
 * The generational half is what makes the returned @ref Tactix::FTactixHandle "FTactixHandle" safe to
 * hold across frames. Every Free bumps the freed slot's generation, so any
 * handle still pointing at that slot is now one generation behind.
 * @ref Tactix::FTactixPool::Resolve "Resolve"
 * compares the two and returns @c nullptr on a mismatch, turning a
 * use-after-free into a clean null instead of a silent alias.
 *
 * @tparam T        Element type. May be non-trivial; the pool placement-news on
 *                  Alloc and runs the destructor on Free (and at pool teardown).
 * @tparam Capacity Slot count, fixed at compile time, at most 65535 so an index
 *                  fits the handle's 16-bit field.
 *
 * @note Not thread-safe. One pool is owned and ticked by one system; cross-thread
 *       access needs external synchronisation.
 */

#pragma once

#include "TactixApi.h"
#include "TactixHandle.h"

#include <cstddef>
#include <cstdint>
#include <new>        // placement new
#include <utility>    // std::forward
#include <type_traits>

namespace Tactix
{
	template <typename T, std::size_t Capacity>
	class FTactixPool
	{
		static_assert(Capacity > 0,      "FTactixPool capacity must be > 0.");
		static_assert(Capacity <= 65535, "FTactixPool capacity is limited to 65535 (16-bit index).");

	public:
		/** @brief Tag-typed handle this pool hands out and resolves. */
		using HandleType = FTactixHandle<T>;
		/** @brief Compile-time slot count, mirrored as a value for callers. */
		static constexpr std::size_t kCapacity    = Capacity;
		/** @brief Free-list terminator. A slot whose @c NextFree is this ends the list. */
		static constexpr uint16_t    kEndOfList   = static_cast<uint16_t>(-1);

		/**
		 * @brief Builds an empty pool with every slot on the free list.
		 *
		 * Slots are chained 0->1->...->last so the first few allocations come back
		 * in index order, and each starts at generation 1 (generation 0 is owned
		 * by the invalid-handle sentinel). No element constructors run here, only
		 * the bookkeeping is initialised.
		 */
		FTactixPool() noexcept
		{
			for (uint16_t i = 0; i < Capacity; ++i)
			{
				Slots[i].NextFree   = (i + 1u < Capacity) ? static_cast<uint16_t>(i + 1u) : kEndOfList;
				Slots[i].Generation = 1;   // first valid handle for every slot has gen=1
				Slots[i].bAlive     = false;
			}
			FreeHead   = 0;
			AliveCount = 0;
		}

		/** @brief Destroys every still-live element. Trivial types skip the loop. */
		~FTactixPool()
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				for (uint16_t i = 0; i < Capacity; ++i)
				{
					if (Slots[i].bAlive)
					{
						AtSlot(i)->~T();
					}
				}
			}
		}

		FTactixPool(const FTactixPool&)            = delete;
		FTactixPool& operator=(const FTactixPool&) = delete;
		FTactixPool(FTactixPool&&)                 = delete;
		FTactixPool& operator=(FTactixPool&&)      = delete;

		/**
		 * @brief Constructs an element in a free slot and returns a handle to it.
		 *
		 * Pops the free list, constructs @p T in place forwarding @p InArgs to its
		 * constructor, and stamps the new handle with the slot's current generation.
		 *
		 * @tparam Args   Constructor argument types, perfectly forwarded.
		 * @param  InArgs Arguments passed straight to `T(...)`.
		 * @return A valid handle, or @ref FTactixHandle::Invalid when the pool is
		 *         full. Check the result; it is marked nodiscard for that reason.
		 */
		template <typename... Args>
		TACTIX_NODISCARD HandleType Alloc(Args&&... InArgs)
		{
			if (FreeHead == kEndOfList)
			{
				return HandleType::Invalid();
			}
			const uint16_t Index = FreeHead;
			FreeHead             = Slots[Index].NextFree;

			::new (static_cast<void*>(Slots[Index].Storage)) T(std::forward<Args>(InArgs)...);
			Slots[Index].bAlive = true;
			++AliveCount;

			return HandleType{Index, Slots[Index].Generation};
		}

		/**
		 * @brief Destroys the element a handle refers to and recycles its slot.
		 *
		 * Validates the handle the same way @ref Resolve does before doing anything,
		 * so a stale or invalid handle is a safe no-op rather than a double free.
		 * On success the slot's generation is bumped (skipping 0 to keep the invalid
		 * sentinel unique), which retires every other handle that still names it.
		 *
		 * @param InHandle Handle to free.
		 * @return True if a live element was destroyed; false if the handle was
		 *         invalid, out of range, already free, or stale.
		 */
		bool Free(HandleType InHandle)
		{
			if (!InHandle.IsValid()) return false;
			const uint32_t Idx32 = InHandle.GetIndex();
			if (Idx32 >= Capacity) return false;
			const uint16_t Index = static_cast<uint16_t>(Idx32);

			if (!Slots[Index].bAlive)                                   return false;
			if (Slots[Index].Generation != InHandle.GetGeneration())    return false;

			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				AtSlot(Index)->~T();
			}
			Slots[Index].bAlive = false;

			// Bump generation, skipping zero so the "invalid handle" sentinel stays unique.
			uint16_t NextGen = static_cast<uint16_t>(Slots[Index].Generation + 1u);
			if (NextGen == 0) NextGen = 1;
			Slots[Index].Generation = NextGen;

			Slots[Index].NextFree = FreeHead;
			FreeHead              = Index;
			--AliveCount;
			return true;
		}

		/**
		 * @brief Turns a handle back into a pointer, or @c nullptr if it's stale.
		 *
		 * This is the one safe dereference path. It rejects the invalid handle,
		 * an out-of-range index, a slot that's currently free, and a slot whose
		 * generation has moved on since the handle was minted.
		 *
		 * @param InHandle Handle to resolve.
		 * @return Pointer to the live element, or @c nullptr.
		 * @warning Treat the pointer as valid only until the next Alloc/Free on
		 *          this pool. Don't cache it across frames; re-resolve the handle.
		 */
		TACTIX_NODISCARD T* Resolve(HandleType InHandle)
		{
			if (!InHandle.IsValid()) return nullptr;
			const uint32_t Idx32 = InHandle.GetIndex();
			if (Idx32 >= Capacity) return nullptr;
			const uint16_t Index = static_cast<uint16_t>(Idx32);
			if (!Slots[Index].bAlive)                                return nullptr;
			if (Slots[Index].Generation != InHandle.GetGeneration()) return nullptr;
			return AtSlot(Index);
		}

		/** @brief Const overload of @ref Resolve with the same validity rules. */
		TACTIX_NODISCARD const T* Resolve(HandleType InHandle) const
		{
			return const_cast<FTactixPool*>(this)->Resolve(InHandle);
		}

		/**
		 * @brief Whether a handle currently resolves to a live element.
		 * @param InHandle Handle to test.
		 * @return True if @ref Resolve would return non-null.
		 */
		TACTIX_NODISCARD bool IsValid(HandleType InHandle) const
		{
			return Resolve(InHandle) != nullptr;
		}

		/** @brief Number of live elements. */
		TACTIX_NODISCARD std::size_t Num()      const { return AliveCount; }
		/** @brief Total slot count (the @p Capacity template argument). */
		TACTIX_NODISCARD std::size_t Capacity_() const { return Capacity; }
		/** @brief True when no free slots remain and the next Alloc would fail. */
		TACTIX_NODISCARD bool        IsFull()   const { return FreeHead == kEndOfList; }

		/**
		 * @brief Visits every live element in slot order.
		 * @tparam F Callable with signature `void(HandleType, T&)`.
		 * @param  Fn Invoked once per live slot with its handle and element.
		 * @warning Don't Alloc or Free on this pool from inside @p Fn; mutating the
		 *          free list mid-walk is not supported.
		 */
		template <typename F>
		void ForEach(F&& Fn)
		{
			for (uint16_t i = 0; i < Capacity; ++i)
			{
				if (Slots[i].bAlive)
				{
					Fn(HandleType{i, Slots[i].Generation}, *AtSlot(i));
				}
			}
		}

	private:
		/** @brief One storage cell plus the bookkeeping that tracks its state. */
		struct Slot
		{
			alignas(T) unsigned char Storage[sizeof(T)]; ///< Raw, correctly-aligned bytes for one @p T.
			uint16_t Generation{1};                      ///< Current generation; bumped on every Free.
			uint16_t NextFree{kEndOfList};               ///< Next free slot when this one is unused.
			bool     bAlive{false};                      ///< Whether @c Storage holds a constructed object.
		};

		Slot     Slots[Capacity];   ///< Backing storage, all slots inline, no heap.
		uint16_t FreeHead{0};       ///< Head of the free list, or @ref kEndOfList when full.
		uint32_t AliveCount{0};     ///< Cached live count so @ref Num is O(1).

		/** @brief Reinterprets a slot's raw bytes as a @p T. Assumes the slot is alive. */
		TACTIX_FORCEINLINE T*       AtSlot(uint16_t Index)       { return reinterpret_cast<T*>(Slots[Index].Storage); }
		/** @brief Const counterpart of @ref AtSlot. */
		TACTIX_FORCEINLINE const T* AtSlot(uint16_t Index) const { return reinterpret_cast<const T*>(Slots[Index].Storage); }
	};
}
