// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixFormation.h
 * @brief Movement formations: per-slot local offsets and the transform that
 *        places them in the world relative to a leader.
 *
 * A formation is a set of slot offsets in formation-local space (+X forward,
 * +Y right, +Z up). The named kinds (line, column, wedge, box, diamond) are
 * generated from a slot count and a spacing; Custom lets you author offsets by
 * hand. @ref Tactix::FTactixFormation::GetSlotWorldPosition "GetSlotWorldPosition" turns a slot into a world
 * position behind/around the leader.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixMath.h"

#include <cstddef>
#include <cstdint>

namespace Tactix
{
	/** @brief The shape a formation lays its slots out in. */
	enum class ETactixFormationKind : uint8_t
	{
		Line,     ///< Side by side, abreast of the leader.
		Column,   ///< Single file behind the leader.
		Wedge,    ///< V shape trailing back to either side.
		Box,      ///< Rectangular block, four per tier.
		Diamond,  ///< Points around the leader, cycling every four slots.
		Custom,   ///< Offsets authored via @ref FTactixFormation::SetCustomSlot.
	};

	/**
	 * @brief One formation slot, stored as an offset in formation-local space.
	 *
	 * Local axes: +X forward, +Y right, +Z up. The world transform treats the
	 * leader's forward as planar (its Z is dropped), so the Z offset rides on top
	 * of the leader's own height, which keeps formations sane on slopes and stairs.
	 */
	struct FTactixFormationSlot
	{
		FTactixVec3 LocalOffset{};  ///< Offset from the leader in formation-local space.
	};

	/**
	 * @brief A formation of up to @p MaxSlots positions.
	 * @tparam MaxSlots Compile-time slot ceiling.
	 */
	template <std::size_t MaxSlots>
	class FTactixFormation
	{
		static_assert(MaxSlots > 0, "FTactixFormation needs at least one slot.");

	public:
		/** @brief Constructs an empty Custom formation with no slots. */
		FTactixFormation() = default;

		/**
		 * @brief Constructs and immediately builds a named formation.
		 * @see Build
		 */
		FTactixFormation(ETactixFormationKind Kind, uint32_t SlotCount, float Spacing)
		{
			Build(Kind, SlotCount, Spacing);
		}

		/**
		 * @brief (Re)generates slot offsets for a named formation kind.
		 * @param Kind    Shape to generate. @c Custom leaves existing slots untouched.
		 * @param Count   Desired slot count; clamped to @p MaxSlots.
		 * @param Spacing Gap between slots in world units; non-positive falls back to 100.
		 */
		void Build(ETactixFormationKind Kind, uint32_t Count, float Spacing)
		{
			this->Kind = Kind;
			this->Spacing = Spacing > 0.0f ? Spacing : 100.0f;
			this->Count = Count > MaxSlots ? static_cast<uint32_t>(MaxSlots) : Count;

			switch (Kind)
			{
			case ETactixFormationKind::Line:    BuildLine();    break;
			case ETactixFormationKind::Column:  BuildColumn();  break;
			case ETactixFormationKind::Wedge:   BuildWedge();   break;
			case ETactixFormationKind::Box:     BuildBox();     break;
			case ETactixFormationKind::Diamond: BuildDiamond(); break;
			case ETactixFormationKind::Custom:  break;  // keep current slots
			}
		}

		/**
		 * @brief Sets one slot's offset by hand and switches the kind to Custom.
		 * @param Index       Slot to write; ignored if it's at or beyond @p MaxSlots.
		 * @param LocalOffset Formation-local offset for the slot.
		 * @note If @p Index is beyond the current count, the count grows to include it.
		 */
		void SetCustomSlot(uint32_t Index, FTactixVec3 LocalOffset)
		{
			if (Index >= MaxSlots) return;
			Slots[Index].LocalOffset = LocalOffset;
			if (Index >= Count) Count = Index + 1u;
			Kind = ETactixFormationKind::Custom;
		}

		/** @brief Number of active slots. */
		uint32_t              GetSlotCount() const { return Count; }
		/** @brief Current formation kind. */
		ETactixFormationKind  GetKind()      const { return Kind; }
		/** @brief Current spacing in world units. */
		float                 GetSpacing()   const { return Spacing; }

		/** @brief Slot by index. No bounds check; @p Index must be < @ref GetSlotCount. */
		const FTactixFormationSlot& GetSlot(uint32_t Index) const { return Slots[Index]; }

		/**
		 * @brief World position for a slot, given where the leader is and faces.
		 *
		 * The leader's forward is flattened onto the XY plane and normalised to build
		 * the local frame (forward / right / up), so the formation rotates with the
		 * leader's heading but stays level. The slot's Z offset is added on top of
		 * the leader's Z, which is what keeps it working across height changes.
		 *
		 * @param SlotIdx       Slot to place; out-of-range returns @p LeaderPos.
		 * @param LeaderPos     Leader world position.
		 * @param LeaderForward Leader facing; a near-zero vector defaults to +X.
		 * @return The slot's world position.
		 */
		FTactixVec3 GetSlotWorldPosition(uint32_t SlotIdx,
		                                 FTactixVec3 LeaderPos,
		                                 FTactixVec3 LeaderForward) const
		{
			if (SlotIdx >= Count) return LeaderPos;

			FTactixVec3 F{ LeaderForward.X, LeaderForward.Y, 0.0f };
			if (F.LengthSquared() < 1e-6f) { F = { 1.0f, 0.0f, 0.0f }; }
			else                           { F = F.Normalized(); }

			// Right = Up x Forward (UE left-handed convention).
			const FTactixVec3 R{ -F.Y, F.X, 0.0f };
			const FTactixVec3 U{  0.0f, 0.0f, 1.0f };

			const FTactixVec3& L = Slots[SlotIdx].LocalOffset;
			return LeaderPos + (F * L.X) + (R * L.Y) + (U * L.Z);
		}

	private:
		FTactixFormationSlot Slots[MaxSlots]{};                    ///< Per-slot local offsets.
		uint32_t             Count{0};                            ///< Active slot count.
		ETactixFormationKind Kind{ETactixFormationKind::Custom};  ///< Current shape.
		float                Spacing{100.0f};                     ///< Current spacing in world units.

		/** @brief Slot 0 on the leader, the rest fanning out left/right abreast. */
		void BuildLine()
		{
			Slots[0].LocalOffset = {};
			for (uint32_t i = 1; i < Count; ++i)
			{
				const int32_t step = static_cast<int32_t>((i + 1) / 2);
				const float   sign = (i & 1u) ? 1.0f : -1.0f;
				Slots[i].LocalOffset = { 0.0f, sign * static_cast<float>(step) * Spacing, 0.0f };
			}
		}

		/** @brief Single file trailing straight back from the leader. */
		void BuildColumn()
		{
			for (uint32_t i = 0; i < Count; ++i)
			{
				Slots[i].LocalOffset = { -static_cast<float>(i) * Spacing, 0.0f, 0.0f };
			}
		}

		/** @brief Leader at the point, slots trailing back to alternating sides (a V). */
		void BuildWedge()
		{
			Slots[0].LocalOffset = {};
			for (uint32_t i = 1; i < Count; ++i)
			{
				const int32_t step = static_cast<int32_t>((i + 1) / 2);
				const float   ySig = (i & 1u) ? -1.0f : 1.0f;  // odd=right, even=left
				Slots[i].LocalOffset = {
					-static_cast<float>(step) * Spacing,
					 ySig * static_cast<float>(step) * Spacing,
					 0.0f
				};
			}
		}

		/** @brief Rectangular block; slots fill in groups of four, each tier further back. */
		void BuildBox()
		{
			const FTactixVec3 Pattern[4] = {
				{ 0.0f,    0.0f,    0.0f },
				{ 0.0f,    Spacing, 0.0f },
				{-Spacing, 0.0f,    0.0f },
				{-Spacing, Spacing, 0.0f },
			};
			for (uint32_t i = 0; i < Count; ++i)
			{
				const uint32_t tier = i / 4u;
				const uint32_t p    = i % 4u;
				FTactixVec3    off  = Pattern[p];
				off.X -= static_cast<float>(tier) * Spacing * 2.0f;
				Slots[i].LocalOffset = off;
			}
		}

		/** @brief Front/right/back/left points around the leader, repeating every four slots. */
		void BuildDiamond()
		{
			const FTactixVec3 Pattern[4] = {
				{  Spacing,  0.0f,   0.0f },
				{  0.0f,     Spacing,0.0f },
				{ -Spacing,  0.0f,   0.0f },
				{  0.0f,    -Spacing,0.0f },
			};
			for (uint32_t i = 0; i < Count; ++i)
			{
				Slots[i].LocalOffset = Pattern[i % 4u];
			}
		}
	};
}
