// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixMath.h"

#include <cstddef>
#include <cstdint>

namespace Tactix
{
	enum class ETactixFormationKind : uint8_t
	{
		Line,
		Column,
		Wedge,
		Box,
		Diamond,
		Custom,
	};

	// Local offset is expressed in formation-local space: +X = forward,
	// +Y = right, +Z = up. World transforms in GetSlotWorldPosition assume
	// leader forward is planar (Z ignored).
	struct FTactixFormationSlot
	{
		FTactixVec3 LocalOffset{};
	};

	template <std::size_t MaxSlots>
	class FTactixFormation
	{
		static_assert(MaxSlots > 0, "FTactixFormation needs at least one slot.");

	public:
		FTactixFormation() = default;

		FTactixFormation(ETactixFormationKind Kind, uint32_t SlotCount, float Spacing)
		{
			Build(Kind, SlotCount, Spacing);
		}

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

		void SetCustomSlot(uint32_t Index, FTactixVec3 LocalOffset)
		{
			if (Index >= MaxSlots) return;
			Slots[Index].LocalOffset = LocalOffset;
			if (Index >= Count) Count = Index + 1u;
			Kind = ETactixFormationKind::Custom;
		}

		uint32_t              GetSlotCount() const { return Count; }
		ETactixFormationKind  GetKind()      const { return Kind; }
		float                 GetSpacing()   const { return Spacing; }

		const FTactixFormationSlot& GetSlot(uint32_t Index) const { return Slots[Index]; }

		// Leader forward is projected to the XY plane; Z stays as the leader's Z
		// plus any per-slot Z offset, so this works on flat terrain and stairs.
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
		FTactixFormationSlot Slots[MaxSlots]{};
		uint32_t             Count{0};
		ETactixFormationKind Kind{ETactixFormationKind::Custom};
		float                Spacing{100.0f};

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

		void BuildColumn()
		{
			for (uint32_t i = 0; i < Count; ++i)
			{
				Slots[i].LocalOffset = { -static_cast<float>(i) * Spacing, 0.0f, 0.0f };
			}
		}

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
