// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixBlackboard.h
 * @brief Typed key-value store with compile-time key hashing: the AI systems'
 *        shared working memory.
 *
 * This plays the same role as UE's @c UBlackboardComponent but is deliberately
 * not a @c UObject and carries no reflection, so it lives in TactixCore with no
 * engine cost. Keys are 32-bit FNV-1a hashes of string literals, minted at
 * compile time by hashing the name (see @ref Tactix::HashFNV1a "HashFNV1a")
 * through the @c _bb user-defined literal:
 *
 * @code
 * using namespace Tactix::Literals;
 *
 * Blackboard.Set("TargetLocation"_bb, FTactixVec3{1, 2, 3});
 *
 * FTactixVec3 Loc;
 * if (Blackboard.Get("TargetLocation"_bb, Loc)) { ... }
 * @endcode
 *
 * Storage is an open-addressed table with linear probing. Distinct keys colliding
 * at 32 bits is vanishingly unlikely, and if two keys ever land in the same
 * bucket the probe walks forward to the next free slot. A hash of 0 is reserved
 * to mean "empty slot", so @ref Tactix::HashFNV1a "HashFNV1a" never returns it.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixMath.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Tactix
{
	/**
	 * @brief Compile-time FNV-1a hash of a C string.
	 * @param Text Null-terminated string to hash.
	 * @return The 32-bit hash, nudged from 0 to 1 so the result can never collide
	 *         with the reserved "empty" marker.
	 */
	constexpr uint32_t HashFNV1a(const char* Text) noexcept
	{
		uint32_t H = 0x811c9dc5u;
		while (*Text != '\0')
		{
			H ^= static_cast<uint32_t>(static_cast<uint8_t>(*Text));
			H *= 0x01000193u;
			++Text;
		}
		return H == 0u ? 1u : H;
	}

	/** @brief A blackboard key: just the hash of its name, comparable and copyable. */
	struct FTactixBBKey
	{
		uint32_t Hash{0};  ///< Key hash; 0 means "no key".

		/** @brief Constructs an invalid (empty) key. */
		constexpr FTactixBBKey() = default;
		/** @brief Wraps a precomputed hash. */
		constexpr explicit FTactixBBKey(uint32_t InHash) : Hash(InHash) {}
		/** @brief Hashes @p Text into a key. */
		constexpr explicit FTactixBBKey(const char* Text) : Hash(HashFNV1a(Text)) {}

		/** @brief Whether this names a real key (hash != 0). */
		constexpr bool IsValid() const { return Hash != 0; }
		constexpr bool operator==(FTactixBBKey R) const { return Hash == R.Hash; }
		constexpr bool operator!=(FTactixBBKey R) const { return Hash != R.Hash; }
	};

	/** @brief Holds the @c _bb literal so callers can opt in with a using-directive. */
	namespace Literals
	{
		/** @brief Turns `"Name"_bb` into an @ref FTactixBBKey at compile time. */
		constexpr FTactixBBKey operator""_bb(const char* Text, std::size_t) noexcept
		{
			return FTactixBBKey(Text);
		}
	}

	/** @brief Discriminator for the value union; also the stored value's type tag. */
	enum class ETactixBBType : uint8_t
	{
		Empty = 0,  ///< No value stored.
		Bool,       ///< @c bool.
		Int,        ///< @c int32_t.
		Float,      ///< @c float.
		Vec3,       ///< @ref FTactixVec3.
		Handle,     ///< A packed handle value (@c uint32_t).
		Ptr,        ///< An opaque @c void*.
	};

	/**
	 * @brief A tagged union holding one blackboard value.
	 *
	 * @c Type says which union member is live. Reads must check the tag (the
	 * accessors do this for you) before touching a member.
	 */
	struct FTactixBBValue
	{
		ETactixBBType Type{ETactixBBType::Empty};  ///< Which member of the union is valid.
		union
		{
			bool         AsBool;         ///< Valid when @c Type == Bool.
			int32_t      AsInt;          ///< Valid when @c Type == Int.
			float        AsFloat;        ///< Valid when @c Type == Float.
			FTactixVec3  AsVec3;         ///< Valid when @c Type == Vec3.
			uint32_t     AsHandlePacked; ///< Valid when @c Type == Handle (packed handle).
			void*        AsPtr;          ///< Valid when @c Type == Ptr.
		};

		/** @brief Constructs an empty value. */
		constexpr FTactixBBValue() : Type(ETactixBBType::Empty), AsPtr(nullptr) {}
	};

	/**
	 * @brief Capacity-independent interface to a blackboard.
	 *
	 * @ref FTactixBlackboard is templated on capacity, which would otherwise leak
	 * into every signature that wants to touch one. This abstract base hides that:
	 * an @ref FTactixAgentContext, a BT service, anything, can hold a
	 * @c FTactixBlackboardRef* without caring how big the concrete table is.
	 *
	 * The typed @ref Get / @ref Set templates dispatch to the per-type virtuals via
	 * `if constexpr`, so callers write `bb.Set(key, value)` and the right virtual
	 * is chosen at compile time.
	 */
	class TACTIXCORE_API FTactixBlackboardRef
	{
	public:
		virtual ~FTactixBlackboardRef() = default;

		/**
		 * @name Typed getters
		 * @brief Read a key only if it exists @e and currently holds this exact type.
		 * @param Key Key to look up.
		 * @param Out Receives the value on success; untouched on failure.
		 * @return True if the key was present with the matching type, false otherwise
		 *         (missing key or type mismatch). A type mismatch is treated as a
		 *         miss, not an error.
		 * @{
		 */
		virtual bool GetBool  (FTactixBBKey Key, bool&          Out) const = 0;
		virtual bool GetInt   (FTactixBBKey Key, int32_t&       Out) const = 0;
		virtual bool GetFloat (FTactixBBKey Key, float&         Out) const = 0;
		virtual bool GetVec3  (FTactixBBKey Key, FTactixVec3&   Out) const = 0;
		virtual bool GetHandle(FTactixBBKey Key, uint32_t&      Out) const = 0;
		virtual bool GetPtr   (FTactixBBKey Key, void*&         Out) const = 0;
		/** @} */

		/**
		 * @name Typed setters
		 * @brief Insert or overwrite a key with a value of this type.
		 * @param Key Key to write.
		 * @param V   Value to store; this also sets the stored type tag.
		 * @note If the table is full and the key is new, the write is silently
		 *       dropped. That is the documented policy; size the blackboard for the
		 *       expected key count.
		 * @{
		 */
		virtual void SetBool  (FTactixBBKey Key, bool               V) = 0;
		virtual void SetInt   (FTactixBBKey Key, int32_t            V) = 0;
		virtual void SetFloat (FTactixBBKey Key, float              V) = 0;
		virtual void SetVec3  (FTactixBBKey Key, const FTactixVec3& V) = 0;
		virtual void SetHandle(FTactixBBKey Key, uint32_t           V) = 0;
		virtual void SetPtr   (FTactixBBKey Key, void*              V) = 0;
		/** @} */

		/** @brief Whether @p Key currently has any value (of any type). */
		virtual bool        Has   (FTactixBBKey Key) const = 0;
		/** @brief Removes @p Key. Returns true if it was present. */
		virtual bool        Remove(FTactixBBKey Key)       = 0;
		/** @brief Removes every entry. */
		virtual void        Clear()                        = 0;
		/** @brief Number of entries currently stored. */
		virtual std::size_t Num()                    const = 0;

		/**
		 * @brief Type-dispatched read; the typed front-end to the @c Get* virtuals.
		 * @tparam T   One of: @c bool, @c int32_t, @c float, @ref FTactixVec3,
		 *             @c uint32_t (packed handle), @c void*. Anything else is a
		 *             compile error.
		 * @param  Key Key to read.
		 * @param  Out Receives the value on success.
		 * @return True on a matching-type hit, false otherwise.
		 */
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

		/**
		 * @brief Type-dispatched write; the typed front-end to the @c Set* virtuals.
		 * @tparam T   Same supported set as @ref Get.
		 * @param  Key Key to write.
		 * @param  V   Value to store.
		 */
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
		/** @brief Compile-time predicate: is @p T a type the blackboard can store? */
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

	/**
	 * @brief Concrete fixed-capacity blackboard backed by an open-addressed table.
	 *
	 * Linear probing with backshift deletion, no heap allocation, no UObject. The
	 * table never resizes; pick @p Capacity comfortably above the number of keys
	 * you expect, since a full table silently drops new inserts.
	 *
	 * @tparam Capacity Slot count. Must be a power of two so the probe wrap is a
	 *         bitmask. Default 64.
	 */
	template <std::size_t Capacity = 64>
	class FTactixBlackboard final : public FTactixBlackboardRef
	{
		static_assert(Capacity > 0,                          "FTactixBlackboard capacity must be > 0.");
		static_assert((Capacity & (Capacity - 1)) == 0,      "FTactixBlackboard capacity must be a power of 2.");

	public:
		/** @brief Compile-time slot count. */
		static constexpr std::size_t kCapacity = Capacity;

		FTactixBlackboard() = default;

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

		/**
		 * @brief Removes a key and repairs the probe chain behind it.
		 *
		 * After clearing the slot, this does a Robin-Hood-style backshift: it walks
		 * forward and drags back any entry that probed past the now-empty slot, so
		 * later @ref Find calls don't terminate early at the hole and miss a key.
		 */
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
		static constexpr std::size_t Mask = Capacity - 1;  ///< Probe wrap mask (capacity is a power of two).
		FTactixBBKey   Keys  [Capacity]{};                 ///< Parallel key array; an invalid key marks an empty slot.
		FTactixBBValue Values[Capacity]{};                 ///< Parallel value array, indexed alongside @c Keys.
		std::size_t    Size{0};                            ///< Live entry count.

		/**
		 * @brief Locates an existing key by linear probing.
		 * @return Its slot index, or @c Capacity if the key isn't present.
		 */
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

		/**
		 * @brief Finds @p Key, or claims the first free slot in its probe chain.
		 * @return The slot to write, or @c Capacity if the table is full and the key
		 *         is new. Claiming a fresh slot also bumps @c Size.
		 */
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

		/**
		 * @brief Shared read body: look up @p Key, verify its type, extract via @p Fn.
		 * @tparam GetFn   Callable `void(const FTactixBBValue&)` that copies out the value.
		 * @param Key      Key to read.
		 * @param Expected Required stored type; a mismatch is reported as a miss.
		 * @param Fn       Extractor invoked with the stored value on a hit.
		 * @return True if the key existed with the expected type and @p Fn ran.
		 */
		template <typename GetFn>
		bool DoGet(FTactixBBKey Key, ETactixBBType Expected, GetFn&& Fn) const
		{
			const std::size_t I = Find(Key);
			if (I == Capacity) return false;
			if (Values[I].Type != Expected) return false;
			Fn(Values[I]);
			return true;
		}

		/**
		 * @brief Shared write body: find/insert @p Key, then populate via @p Fn.
		 * @tparam SetFn Callable `void(FTactixBBValue&)` that sets the tag and member.
		 * @note Silently does nothing when the table is full and the key is new.
		 */
		template <typename SetFn>
		void DoSet(FTactixBBKey Key, SetFn&& Fn)
		{
			const std::size_t I = FindOrInsert(Key);
			if (I == Capacity) return; // table full — silently drop (documented policy)
			Fn(Values[I]);
		}
	};
}
