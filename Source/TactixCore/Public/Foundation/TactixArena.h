// Copyright Sleak Software. All Rights Reserved.
//
// TactixArena — bump allocator for short-lived scratch memory.
//
// Designed for plan temporaries: the GOAP and HTN planners reset the arena
// between planning passes so every pass starts with a zero cost, zero
// fragmentation allocator.
//
// Semantics:
//   • Alloc(size, align) bumps a cursor inside a single pre-allocated buffer.
//   • Emplace<T>(args...) allocates + placement-news a trivially-managed T.
//     Destructors are NOT called on Reset() — the arena is for POD-ish data.
//   • Reset() rewinds the cursor to zero. All pointers into the arena become
//     invalid; the caller is responsible for dropping them.
//
// The backing buffer is heap-allocated once at construction. This is the
// ONLY heap allocation the arena performs; Alloc/Reset are hot-path safe.

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
		explicit FTactixArena(std::size_t CapacityBytes)
			: Buffer(CapacityBytes > 0 ? new unsigned char[CapacityBytes] : nullptr)
			, Capacity(CapacityBytes)
			, Used(0)
		{
		}

		~FTactixArena() { delete[] Buffer; }

		FTactixArena(const FTactixArena&)            = delete;
		FTactixArena& operator=(const FTactixArena&) = delete;

		FTactixArena(FTactixArena&& Other) noexcept
			: Buffer(Other.Buffer), Capacity(Other.Capacity), Used(Other.Used)
		{
			Other.Buffer   = nullptr;
			Other.Capacity = 0;
			Other.Used     = 0;
		}

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

		TACTIX_NODISCARD void* Alloc(std::size_t Size, std::size_t Align) noexcept
		{
			if (Align == 0) Align = 1;
			const std::size_t Aligned = (Used + Align - 1) & ~(Align - 1);
			if (Aligned + Size > Capacity) return nullptr;
			void* Ptr = Buffer + Aligned;
			Used      = Aligned + Size;
			return Ptr;
		}

		template <typename T, typename... Args>
		TACTIX_NODISCARD T* Emplace(Args&&... InArgs)
		{
			if (void* Ptr = Alloc(sizeof(T), alignof(T)))
			{
				return ::new (Ptr) T(std::forward<Args>(InArgs)...);
			}
			return nullptr;
		}

		// Reserve an uninitialised array of N contiguous T objects.
		template <typename T>
		TACTIX_NODISCARD T* EmplaceArrayUninitialised(std::size_t N)
		{
			if (N == 0) return nullptr;
			return static_cast<T*>(Alloc(sizeof(T) * N, alignof(T)));
		}

		void Reset() noexcept { Used = 0; }

		TACTIX_NODISCARD std::size_t GetUsed()      const noexcept { return Used; }
		TACTIX_NODISCARD std::size_t GetCapacity()  const noexcept { return Capacity; }
		TACTIX_NODISCARD std::size_t GetRemaining() const noexcept { return Capacity - Used; }

	private:
		unsigned char* Buffer;
		std::size_t    Capacity;
		std::size_t    Used;
	};
}
