// Copyright Sleak Software. All Rights Reserved.
//
// TactixPool — fixed-capacity, generation-tracking pool allocator.
//
// Allocates storage for `Capacity` objects up-front. All runtime Alloc/Free
// operations are O(1) and perform zero heap allocations (per the "no new on
// hot path" rule). Frees are cheap because a free-list thread through the
// `NextFree` field of each slot gives us a stack of available indices.
//
// Each slot maintains its own generation counter. Every Free increments the
// generation so any lingering handle to that slot becomes stale — Resolve()
// compares the handle's generation against the slot's current generation
// before returning a pointer.

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
		using HandleType = FTactixHandle<T>;
		static constexpr std::size_t kCapacity    = Capacity;
		static constexpr uint16_t    kEndOfList   = static_cast<uint16_t>(-1);

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

		TACTIX_NODISCARD const T* Resolve(HandleType InHandle) const
		{
			return const_cast<FTactixPool*>(this)->Resolve(InHandle);
		}

		TACTIX_NODISCARD bool IsValid(HandleType InHandle) const
		{
			return Resolve(InHandle) != nullptr;
		}

		TACTIX_NODISCARD std::size_t Num()      const { return AliveCount; }
		TACTIX_NODISCARD std::size_t Capacity_() const { return Capacity; }
		TACTIX_NODISCARD bool        IsFull()   const { return FreeHead == kEndOfList; }

		// Walk all live entries. Functor signature: void(HandleType, T&).
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
		struct Slot
		{
			alignas(T) unsigned char Storage[sizeof(T)];
			uint16_t Generation{1};
			uint16_t NextFree{kEndOfList};
			bool     bAlive{false};
		};

		Slot     Slots[Capacity];
		uint16_t FreeHead{0};
		uint32_t AliveCount{0};

		TACTIX_FORCEINLINE T*       AtSlot(uint16_t Index)       { return reinterpret_cast<T*>(Slots[Index].Storage); }
		TACTIX_FORCEINLINE const T* AtSlot(uint16_t Index) const { return reinterpret_cast<const T*>(Slots[Index].Storage); }
	};
}
