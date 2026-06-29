// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixArena.h
 * @brief Bump (linear) allocator for short-lived scratch memory.
 *
 * Built for plan temporaries. The GOAP and HTN planners allocate a swarm of tiny
 * nodes during a single planning pass, then throw them all away at once. An arena
 * makes that pattern essentially free: allocation is a pointer bump, and freeing
 * the whole pass is a single @ref Tactix::FTactixArena::Reset "Reset" that rewinds the cursor. There is no
 * per-object free and no fragmentation, because individual objects are never
 * returned.
 *
 * The flip side, and the thing to keep in mind: @ref Tactix::FTactixArena::Reset "Reset" does @b not run
 * destructors. The arena is meant for trivially destructible, POD-ish data. If
 * you place something with a meaningful destructor in here, that destructor never
 * runs. After a @ref Tactix::FTactixArena::Reset "Reset" every pointer previously handed out is dangling, and
 * dropping those pointers is the caller's job.
 *
 * The backing buffer is heap-allocated exactly once, in the constructor. That is
 * the only allocation the arena ever performs; @ref Tactix::FTactixArena::Alloc "Alloc"
 * and @ref Tactix::FTactixArena::Reset "Reset" are safe on the hot path.
 */

#pragma once

#include "TactixApi.h"

#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>

namespace Tactix
{
	class TACTIXCORE_API FTactixArena
	{
	public:
		/**
		 * @brief Allocates the fixed backing buffer.
		 * @param CapacityBytes Total bytes the arena can hand out before @ref Alloc
		 *        starts failing. A capacity of 0 leaves the arena permanently empty
		 *        (every Alloc returns @c nullptr), which is legal but rarely useful.
		 */
		explicit FTactixArena(std::size_t CapacityBytes)
			: Buffer(CapacityBytes > 0 ? new unsigned char[CapacityBytes] : nullptr)
			, Capacity(CapacityBytes)
			, Used(0)
		{
		}

		/** @brief Releases the backing buffer. Does not destroy arena contents. */
		~FTactixArena() { delete[] Buffer; }

		FTactixArena(const FTactixArena&)            = delete;
		FTactixArena& operator=(const FTactixArena&) = delete;

		/** @brief Steals @p Other's buffer and leaves it empty. */
		FTactixArena(FTactixArena&& Other) noexcept
			: Buffer(Other.Buffer), Capacity(Other.Capacity), Used(Other.Used)
		{
			Other.Buffer   = nullptr;
			Other.Capacity = 0;
			Other.Used     = 0;
		}

		/** @brief Frees this arena's buffer, then steals @p Other's. */
		FTactixArena& operator=(FTactixArena&& Other) noexcept
		{
			if (this != &Other)
			{
				delete[] Buffer;
				Buffer         = Other.Buffer;
				Capacity       = Other.Capacity;
				Used           = Other.Used;
				Other.Buffer   = nullptr;
				Other.Capacity = 0;
				Other.Used     = 0;
			}
			return *this;
		}

		/**
		 * @brief Carves @p Size bytes off the buffer at the requested alignment.
		 *
		 * Rounds the cursor up to @p Align, then advances it by @p Size. Nothing is
		 * constructed; the bytes are raw.
		 *
		 * @param Size  Number of bytes to reserve.
		 * @param Align Required alignment in bytes. Zero is treated as 1. Must be a
		 *              power of two; the round-up uses a bitmask, not a modulus.
		 * @return Pointer to the reserved bytes, or @c nullptr if the remaining
		 *         space (after alignment) can't fit @p Size.
		 */
		TACTIX_NODISCARD void* Alloc(std::size_t Size, std::size_t Align) noexcept
		{
			if (Align == 0) Align = 1;
			const std::size_t Aligned = (Used + Align - 1) & ~(Align - 1);
			if (Aligned + Size > Capacity) return nullptr;
			void* Ptr = Buffer + Aligned;
			Used      = Aligned + Size;
			return Ptr;
		}

		/**
		 * @brief Allocates and constructs a single @p T in the arena.
		 * @tparam T      Type to construct. Its destructor will never be called.
		 * @tparam Args   Constructor argument types, forwarded.
		 * @param  InArgs Arguments forwarded to `T(...)`.
		 * @return Pointer to the new object, or @c nullptr if the arena is full.
		 */
		template <typename T, typename... Args>
		TACTIX_NODISCARD T* Emplace(Args&&... InArgs)
		{
			if (void* Ptr = Alloc(sizeof(T), alignof(T)))
			{
				return ::new (Ptr) T(std::forward<Args>(InArgs)...);
			}
			return nullptr;
		}

		/**
		 * @brief Reserves space for @p N contiguous @p T without constructing any.
		 * @tparam T Element type.
		 * @param  N Element count.
		 * @return Pointer to uninitialised storage for @p N elements, @c nullptr if
		 *         @p N is 0 or the arena can't fit the request.
		 * @warning The memory is uninitialised. Construct elements yourself before
		 *          reading them.
		 */
		template <typename T>
		TACTIX_NODISCARD T* EmplaceArrayUninitialised(std::size_t N)
		{
			if (N == 0) return nullptr;
			return static_cast<T*>(Alloc(sizeof(T) * N, alignof(T)));
		}

		/**
		 * @brief Rewinds the cursor so the whole buffer is available again.
		 * @warning Every pointer previously returned by this arena is invalid after
		 *          this call. No destructors run. Drop those pointers first.
		 */
		void Reset() noexcept { Used = 0; }

		/** @brief Bytes handed out so far, including alignment padding. */
		TACTIX_NODISCARD std::size_t GetUsed()      const noexcept { return Used; }
		/** @brief Total buffer size in bytes. */
		TACTIX_NODISCARD std::size_t GetCapacity()  const noexcept { return Capacity; }
		/** @brief Bytes still free, ignoring any alignment the next Alloc may need. */
		TACTIX_NODISCARD std::size_t GetRemaining() const noexcept { return Capacity - Used; }

	private:
		unsigned char* Buffer;   ///< Heap buffer allocated once at construction.
		std::size_t    Capacity; ///< Size of @c Buffer in bytes.
		std::size_t    Used;     ///< Bump cursor: bytes consumed from the front.
	};
}
