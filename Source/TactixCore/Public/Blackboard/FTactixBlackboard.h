// Copyright Sleak Software. All Rights Reserved.
//
// FTactixBlackboard — typed key-value store with compile-time key hashing.
//
// The blackboard is the shared working memory for AI systems. Unlike UE's
// UBlackboardComponent, this store is not a UObject and has no reflection
// dependency — making it usable in TactixCore with zero engine overhead.
//
// Keys are FNV-1a hashes of string literals, created with the `_bb` UDL:
//
//     using namespace Tactix::Literals;
//     Blackboard.Set("TargetLocation"_bb, FTactixVec3{1, 2, 3});
//     FTactixVec3 Loc;
//     if (Blackboard.Get("TargetLocation"_bb, Loc)) { ... }
//
// Collisions between distinct keys are astronomically unlikely at 32-bit
// resolution, but the open-addressed table walks to the next empty slot if
// two keys happen to hash to the same bucket. Hash == 0 is reserved as "empty".

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixMath.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Tactix
{
	// ---- Compile-time FNV-1a string hash -----------------------------------

	constexpr uint32_t HashFNV1a(const char* Text) noexcept
	{
		uint32_t H = 0x811c9dc5u;
		while (*Text != '\0')
		{
			H ^= static_cast<uint32_t>(static_cast<uint8_t>(*Text));
			H *= 0x01000193u;
			++Text;
		}
		// We reserve 0 as "empty"; nudge colliding hashes to 1 so no real key
		// is ever indistinguishable from an empty slot.
		return H == 0u ? 1u : H;
	}

	struct FTactixBBKey
	{
		uint32_t Hash{0};

		constexpr FTactixBBKey() = default;
		constexpr explicit FTactixBBKey(uint32_t InHash) : Hash(InHash) {}
		constexpr explicit FTactixBBKey(const char* Text) : Hash(HashFNV1a(Text)) {}

		constexpr bool IsValid() const { return Hash != 0; }
		constexpr bool operator==(FTactixBBKey R) const { return Hash == R.Hash; }
		constexpr bool operator!=(FTactixBBKey R) const { return Hash != R.Hash; }
	};

	namespace Literals
	{
		constexpr FTactixBBKey operator""_bb(const char* Text, std::size_t) noexcept
		{
			return FTactixBBKey(Text);
		}
	}

	// ---- Tagged-union value ------------------------------------------------

	enum class ETactixBBType : uint8_t
	{
		Empty = 0,
		Bool,
		Int,
		Float,
		Vec3,
		Handle,
		Ptr,
	};

	struct FTactixBBValue
	{
		ETactixBBType Type{ETactixBBType::Empty};
		union
		{
			bool         AsBool;
			int32_t      AsInt;
			float        AsFloat;
			FTactixVec3  AsVec3;
			uint32_t     AsHandlePacked;
			void*        AsPtr;
		};

		constexpr FTactixBBValue() : Type(ETactixBBType::Empty), AsPtr(nullptr) {}
	};

	// ---- Capacity-independent abstract base --------------------------------

	class TACTIXCORE_API FTactixBlackboardRef
	{
	public:
		virtual ~FTactixBlackboardRef() = default;

		virtual bool GetBool  (FTactixBBKey Key, bool&          Out) const = 0;
		virtual bool GetInt   (FTactixBBKey Key, int32_t&       Out) const = 0;
		virtual bool GetFloat (FTactixBBKey Key, float&         Out) const = 0;
		virtual bool GetVec3  (FTactixBBKey Key, FTactixVec3&   Out) const = 0;
		virtual bool GetHandle(FTactixBBKey Key, uint32_t&      Out) const = 0;
		virtual bool GetPtr   (FTactixBBKey Key, void*&         Out) const = 0;

		virtual void SetBool  (FTactixBBKey Key, bool               V) = 0;
		virtual void SetInt   (FTactixBBKey Key, int32_t            V) = 0;
		virtual void SetFloat (FTactixBBKey Key, float              V) = 0;
		virtual void SetVec3  (FTactixBBKey Key, const FTactixVec3& V) = 0;
		virtual void SetHandle(FTactixBBKey Key, uint32_t           V) = 0;
		virtual void SetPtr   (FTactixBBKey Key, void*              V) = 0;

		virtual bool        Has   (FTactixBBKey Key) const = 0;
		virtual bool        Remove(FTactixBBKey Key)       = 0;
		virtual void        Clear()                        = 0;
		virtual std::size_t Num()                    const = 0;

		// ---- Templated convenience (non-virtual, dispatches via `if constexpr`) --

		template <typename T>
		bool Get(FTactixBBKey Key, T& Out) const
		{
			static_assert(SupportsType<T>(), "FTactixBlackboard: unsupported value type.");
			if constexpr (std::is_same_v<T, bool>)         return GetBool  (Key, Out);
			else if constexpr (std::is_same_v<T, int32_t>) return GetInt   (Key, Out);
			else if constexpr (std::is_same_v<T, float>)   return GetFloat (Key, Out);
			else if constexpr (std::is_same_v<T, FTactixVec3>)   return GetVec3 (Key, Out);
			else if constexpr (std::is_same_v<T, uint32_t>)      return GetHandle(Key, Out);
			else if constexpr (std::is_same_v<T, void*>)         return GetPtr  (Key, Out);
		}

		template <typename T>
		void Set(FTactixBBKey Key, const T& V)
		{
			static_assert(SupportsType<T>(), "FTactixBlackboard: unsupported value type.");
			if constexpr (std::is_same_v<T, bool>)         SetBool  (Key, V);
			else if constexpr (std::is_same_v<T, int32_t>) SetInt   (Key, V);
			else if constexpr (std::is_same_v<T, float>)   SetFloat (Key, V);
			else if constexpr (std::is_same_v<T, FTactixVec3>)   SetVec3 (Key, V);
			else if constexpr (std::is_same_v<T, uint32_t>)      SetHandle(Key, V);
			else if constexpr (std::is_same_v<T, void*>)         SetPtr  (Key, V);
		}

	private:
		template <typename T>
		static constexpr bool SupportsType()
		{
			return  std::is_same_v<T, bool>
			     || std::is_same_v<T, int32_t>
			     || std::is_same_v<T, float>
			     || std::is_same_v<T, FTactixVec3>
			     || std::is_same_v<T, uint32_t>
			     || std::is_same_v<T, void*>;
		}
	};

	// ---- Concrete open-addressed implementation ----------------------------

	template <std::size_t Capacity = 64>
	class FTactixBlackboard final : public FTactixBlackboardRef
	{
		static_assert(Capacity > 0,                          "FTactixBlackboard capacity must be > 0.");
		static_assert((Capacity & (Capacity - 1)) == 0,      "FTactixBlackboard capacity must be a power of 2.");

	public:
		static constexpr std::size_t kCapacity = Capacity;

		FTactixBlackboard() = default;

		// ---- Virtual overrides -----------------------------------------------

		bool GetBool  (FTactixBBKey Key, bool&        Out) const override { return DoGet(Key, ETactixBBType::Bool,   [&](const FTactixBBValue& V){ Out = V.AsBool;  }); }
		bool GetInt   (FTactixBBKey Key, int32_t&     Out) const override { return DoGet(Key, ETactixBBType::Int,    [&](const FTactixBBValue& V){ Out = V.AsInt;   }); }
		bool GetFloat (FTactixBBKey Key, float&       Out) const override { return DoGet(Key, ETactixBBType::Float,  [&](const FTactixBBValue& V){ Out = V.AsFloat; }); }
		bool GetVec3  (FTactixBBKey Key, FTactixVec3& Out) const override { return DoGet(Key, ETactixBBType::Vec3,   [&](const FTactixBBValue& V){ Out = V.AsVec3;  }); }
		bool GetHandle(FTactixBBKey Key, uint32_t&    Out) const override { return DoGet(Key, ETactixBBType::Handle, [&](const FTactixBBValue& V){ Out = V.AsHandlePacked; }); }
		bool GetPtr   (FTactixBBKey Key, void*&       Out) const override { return DoGet(Key, ETactixBBType::Ptr,    [&](const FTactixBBValue& V){ Out = V.AsPtr;   }); }

		void SetBool  (FTactixBBKey Key, bool               V) override { DoSet(Key, [&](FTactixBBValue& Dst){ Dst.Type = ETactixBBType::Bool;   Dst.AsBool  = V; }); }
		void SetInt   (FTactixBBKey Key, int32_t            V) override { DoSet(Key, [&](FTactixBBValue& Dst){ Dst.Type = ETactixBBType::Int;    Dst.AsInt   = V; }); }
		void SetFloat (FTactixBBKey Key, float              V) override { DoSet(Key, [&](FTactixBBValue& Dst){ Dst.Type = ETactixBBType::Float;  Dst.AsFloat = V; }); }
		void SetVec3  (FTactixBBKey Key, const FTactixVec3& V) override { DoSet(Key, [&](FTactixBBValue& Dst){ Dst.Type = ETactixBBType::Vec3;   Dst.AsVec3  = V; }); }
		void SetHandle(FTactixBBKey Key, uint32_t           V) override { DoSet(Key, [&](FTactixBBValue& Dst){ Dst.Type = ETactixBBType::Handle; Dst.AsHandlePacked = V; }); }
		void SetPtr   (FTactixBBKey Key, void*              V) override { DoSet(Key, [&](FTactixBBValue& Dst){ Dst.Type = ETactixBBType::Ptr;    Dst.AsPtr   = V; }); }

		bool Has(FTactixBBKey Key) const override { return Find(Key) != Capacity; }

		bool Remove(FTactixBBKey Key) override
		{
			std::size_t Hole = Find(Key);
			if (Hole == Capacity) return false;

			Keys[Hole]   = {};
			Values[Hole] = {};
			--Size;

			// Open-addressed backshift: walk forward and move any entry whose
			// "home" slot sits cyclically in [Home, J) across the hole — if
			// yes, dragging it back to the hole preserves probe invariants.
			std::size_t J = (Hole + 1) & Mask;
			while (Keys[J].IsValid())
			{
				const std::size_t Home = Keys[J].Hash & Mask;
				const bool InRange = (Home <= J) ? (Hole >= Home && Hole < J)
				                                 : (Hole >= Home || Hole < J);
				if (!InRange) break;

				Keys[Hole]   = Keys[J];
				Values[Hole] = Values[J];
				Keys[J]      = {};
				Values[J]    = {};
				Hole         = J;
				J            = (J + 1) & Mask;
			}
			return true;
		}

		void Clear() override
		{
			for (std::size_t i = 0; i < Capacity; ++i) { Keys[i] = {}; Values[i] = {}; }
			Size = 0;
		}

		std::size_t Num() const override { return Size; }

	private:
		static constexpr std::size_t Mask = Capacity - 1;
		FTactixBBKey   Keys  [Capacity]{};
		FTactixBBValue Values[Capacity]{};
		std::size_t    Size{0};

		// Open-addressed find. Returns index or Capacity if absent.
		std::size_t Find(FTactixBBKey Key) const
		{
			if (!Key.IsValid()) return Capacity;
			std::size_t I = Key.Hash & Mask;
			for (std::size_t Probes = 0; Probes < Capacity; ++Probes)
			{
				if (!Keys[I].IsValid())     return Capacity; // empty slot = not found
				if (Keys[I].Hash == Key.Hash) return I;
				I = (I + 1) & Mask;
			}
			return Capacity;
		}

		// Find an existing slot or the first empty slot for insertion.
		std::size_t FindOrInsert(FTactixBBKey Key)
		{
			if (!Key.IsValid()) return Capacity;
			std::size_t I = Key.Hash & Mask;
			for (std::size_t Probes = 0; Probes < Capacity; ++Probes)
			{
				if (!Keys[I].IsValid())
				{
					Keys[I] = Key;
					++Size;
					return I;
				}
				if (Keys[I].Hash == Key.Hash) return I;
				I = (I + 1) & Mask;
			}
			return Capacity; // table full
		}

		template <typename GetFn>
		bool DoGet(FTactixBBKey Key, ETactixBBType Expected, GetFn&& Fn) const
		{
			const std::size_t I = Find(Key);
			if (I == Capacity) return false;
			if (Values[I].Type != Expected) return false;
			Fn(Values[I]);
			return true;
		}

		template <typename SetFn>
		void DoSet(FTactixBBKey Key, SetFn&& Fn)
		{
			const std::size_t I = FindOrInsert(Key);
			if (I == Capacity) return; // table full — silently drop (documented policy)
			Fn(Values[I]);
		}
	};
}
