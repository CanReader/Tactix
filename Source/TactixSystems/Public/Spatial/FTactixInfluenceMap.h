// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixInfluenceMap.h
 * @brief Multi-channel 2D scalar field for spatial reasoning (danger, control,
 *        resource, ...).
 *
 * Each channel is a grid of floats laid over the world. Agents and events stamp
 * values into it (@ref Tactix::FTactixInfluenceMap::Stamp "Stamp"), the values fade over time
 * (@ref Tactix::FTactixInfluenceMap::Decay "Decay"), and AI samples the field to ask "how
 * dangerous is here?" or "where do we hold the most control?". All channels share
 * one contiguous allocation, stored back to back, so a per-channel pass scans
 * linear memory.
 *
 * @tparam ChannelCount Number of independent layers. Must be at least 1.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixMath.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Tactix
{
	template <std::size_t ChannelCount>
	class FTactixInfluenceMap
	{
		static_assert(ChannelCount > 0, "FTactixInfluenceMap needs at least one channel.");

	public:
		/**
		 * @brief Allocates a zeroed map.
		 * @param InWidth    Column count; non-positive yields an empty map.
		 * @param InHeight   Row count; non-positive yields an empty map.
		 * @param InCellSize World size of a cell; non-positive is clamped to 1.
		 * @param InOrigin   World position of the map's lower-left corner.
		 */
		FTactixInfluenceMap(int32_t InWidth, int32_t InHeight, float InCellSize, FTactixVec2 InOrigin = {})
			: Width(InWidth)
			, Height(InHeight)
			, CellSize(InCellSize > 0.0f ? InCellSize : 1.0f)
			, Origin(InOrigin)
			, Data(InWidth > 0 && InHeight > 0
			       ? new float[static_cast<std::size_t>(InWidth) * static_cast<std::size_t>(InHeight) * ChannelCount]{}
			       : nullptr)
		{
		}

		/** @brief Frees the shared channel buffer. */
		~FTactixInfluenceMap() { delete[] Data; }

		FTactixInfluenceMap(const FTactixInfluenceMap&)            = delete;
		FTactixInfluenceMap& operator=(const FTactixInfluenceMap&) = delete;

		/** @brief Steals @p O's buffer, leaving it empty. */
		FTactixInfluenceMap(FTactixInfluenceMap&& O) noexcept
			: Width(O.Width), Height(O.Height), CellSize(O.CellSize), Origin(O.Origin), Data(O.Data)
		{
			O.Data   = nullptr;
			O.Width  = 0;
			O.Height = 0;
		}

		/** @brief Frees this map's buffer, then steals @p O's. */
		FTactixInfluenceMap& operator=(FTactixInfluenceMap&& O) noexcept
		{
			if (this != &O)
			{
				delete[] Data;
				Width = O.Width; Height = O.Height; CellSize = O.CellSize; Origin = O.Origin; Data = O.Data;
				O.Data = nullptr; O.Width = 0; O.Height = 0;
			}
			return *this;
		}

		/** @brief Column count. */
		int32_t     GetWidth()    const { return Width; }
		/** @brief Row count. */
		int32_t     GetHeight()   const { return Height; }
		/** @brief World size of one cell. */
		float       GetCellSize() const { return CellSize; }
		/** @brief World corner of cell (0, 0). */
		FTactixVec2 GetOrigin()   const { return Origin; }
		/** @brief Cells per channel (`Width * Height`). */
		std::size_t NumCells()    const { return static_cast<std::size_t>(Width) * static_cast<std::size_t>(Height); }

		/**
		 * @brief Adds a circular, linearly-falling-off contribution to a channel.
		 *
		 * Each affected cell gains `Value * max(0, 1 - distance/Radius)`, so the peak
		 * is at @p World and it fades to nothing at @p Radius. Stamps accumulate;
		 * use @ref Decay or @ref Clear to drain them.
		 *
		 * @param Channel Channel index; out-of-range or an empty map is a no-op.
		 * @param World   World-space center of the stamp.
		 * @param Radius  Falloff radius in world units; non-positive is a no-op.
		 * @param Value   Peak contribution at the center (may be negative).
		 */
		void Stamp(std::size_t Channel, FTactixVec2 World, float Radius, float Value)
		{
			if (Channel >= ChannelCount || Data == nullptr || Radius <= 0.0f) return;

			int32_t CX, CY;
			WorldToCell(World, CX, CY);
			const int32_t CellRadius = static_cast<int32_t>(std::ceil(Radius / CellSize));
			const int32_t X0 = Max(0,           CX - CellRadius);
			const int32_t X1 = Min(Width  - 1,  CX + CellRadius);
			const int32_t Y0 = Max(0,           CY - CellRadius);
			const int32_t Y1 = Min(Height - 1,  CY + CellRadius);

			float* Ch = ChannelData(Channel);
			const float InvR = 1.0f / Radius;

			for (int32_t y = Y0; y <= Y1; ++y)
			for (int32_t x = X0; x <= X1; ++x)
			{
				const FTactixVec2 C = CellCenter(x, y);
				const float DX = C.X - World.X;
				const float DY = C.Y - World.Y;
				const float D  = std::sqrt(DX * DX + DY * DY);
				if (D >= Radius) continue;
				Ch[Idx(x, y)] += Value * (1.0f - D * InvR);
			}
		}

		/**
		 * @brief Multiplicatively fades a channel toward zero.
		 * @param Channel Channel to decay; out-of-range or empty map is a no-op.
		 * @param Rate    Fraction removed per call, clamped to [0, 1]. 0 keeps the
		 *                channel as-is, 1 clears it (`cell *= 1 - Rate`).
		 */
		void Decay(std::size_t Channel, float Rate)
		{
			if (Channel >= ChannelCount || Data == nullptr) return;
			const float Factor = 1.0f - Clamp(Rate, 0.0f, 1.0f);
			float* Ch = ChannelData(Channel);
			const std::size_t N = NumCells();
			for (std::size_t i = 0; i < N; ++i) Ch[i] *= Factor;
		}

		/** @brief Zeroes one channel. No-op for an out-of-range channel or empty map. */
		void Clear(std::size_t Channel)
		{
			if (Channel >= ChannelCount || Data == nullptr) return;
			float* Ch = ChannelData(Channel);
			const std::size_t N = NumCells();
			for (std::size_t i = 0; i < N; ++i) Ch[i] = 0.0f;
		}

		/**
		 * @brief Reads a channel at the cell containing @p World (no interpolation).
		 * @return The cell value, or 0 for an out-of-bounds sample, bad channel, or
		 *         empty map.
		 */
		float SampleNearest(std::size_t Channel, FTactixVec2 World) const
		{
			if (Channel >= ChannelCount || Data == nullptr) return 0.0f;
			int32_t X, Y;
			WorldToCell(World, X, Y);
			if (X < 0 || Y < 0 || X >= Width || Y >= Height) return 0.0f;
			return ChannelData(Channel)[Idx(X, Y)];
		}

		/**
		 * @brief Reads a channel with bilinear interpolation between the four
		 *        surrounding cell centers.
		 *
		 * Smoother than @ref SampleNearest, which matters when the value feeds
		 * gradients or steering. Samples that fall partly outside the grid treat the
		 * missing neighbours as 0.
		 *
		 * @return The interpolated value, or 0 for a bad channel or empty map.
		 */
		float SampleBilinear(std::size_t Channel, FTactixVec2 World) const
		{
			if (Channel >= ChannelCount || Data == nullptr) return 0.0f;

			const float U = (World.X - Origin.X) / CellSize - 0.5f;
			const float V = (World.Y - Origin.Y) / CellSize - 0.5f;
			const int32_t X0 = static_cast<int32_t>(std::floor(U));
			const int32_t Y0 = static_cast<int32_t>(std::floor(V));
			const float FX = U - static_cast<float>(X0);
			const float FY = V - static_cast<float>(Y0);

			const float* Ch = ChannelData(Channel);
			auto At = [&](int32_t X, int32_t Y) -> float
			{
				if (X < 0 || Y < 0 || X >= Width || Y >= Height) return 0.0f;
				return Ch[Idx(X, Y)];
			};

			const float A = At(X0,     Y0    );
			const float B = At(X0 + 1, Y0    );
			const float C = At(X0,     Y0 + 1);
			const float D = At(X0 + 1, Y0 + 1);
			const float Top    = A + (B - A) * FX;
			const float Bottom = C + (D - C) * FX;
			return Top + (Bottom - Top) * FY;
		}

		/** @brief Const pointer to a channel's contiguous cell block. */
		const float* ChannelData(std::size_t Channel) const { return Data + Channel * NumCells(); }
		/** @brief Mutable pointer to a channel's contiguous cell block. */
		float*       ChannelData(std::size_t Channel)       { return Data + Channel * NumCells(); }

		/**
		 * @brief Converts a world position to cell coordinates (may be out of range).
		 * @param World World-space point.
		 * @param OutX  Receives the column.
		 * @param OutY  Receives the row.
		 */
		void WorldToCell(FTactixVec2 World, int32_t& OutX, int32_t& OutY) const
		{
			OutX = static_cast<int32_t>(std::floor((World.X - Origin.X) / CellSize));
			OutY = static_cast<int32_t>(std::floor((World.Y - Origin.Y) / CellSize));
		}

		/** @brief World position of a cell's center. */
		FTactixVec2 CellCenter(int32_t X, int32_t Y) const
		{
			return { Origin.X + (static_cast<float>(X) + 0.5f) * CellSize,
			         Origin.Y + (static_cast<float>(Y) + 0.5f) * CellSize };
		}

	private:
		int32_t     Width{0};       ///< Column count.
		int32_t     Height{0};      ///< Row count.
		float       CellSize{1.0f}; ///< World size of one cell.
		FTactixVec2 Origin{};       ///< World corner of cell (0, 0).
		float*      Data{nullptr};  ///< All channels back to back: channel c starts at c * NumCells().

		/** @brief Flattens cell coordinates to a within-channel offset. No bounds check. */
		std::size_t Idx(int32_t X, int32_t Y) const
		{
			return static_cast<std::size_t>(Y) * static_cast<std::size_t>(Width) + static_cast<std::size_t>(X);
		}
	};
}
