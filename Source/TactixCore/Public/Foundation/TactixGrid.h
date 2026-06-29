// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixGrid.h
 * @brief Engine-agnostic 2D grid with world<->cell coordinate conversion.
 *
 * A flat, row-major array of cells plus the small amount of math needed to map
 * between world positions and integer cell coordinates. The influence map and
 * other L3 systems build on this: row-major storage keeps stamp and blur passes
 * to a tight, cache-friendly inner loop. The single allocation happens in the
 * constructor; everything afterwards just indexes the existing buffer.
 *
 * @tparam T Cell type. Must be default-constructible because the whole grid is
 *           value-initialised up front.
 */

#pragma once

#include "TactixApi.h"
#include "TactixMath.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace Tactix
{
	template <typename T>
	class FTactixGrid
	{
		static_assert(std::is_default_constructible_v<T>, "FTactixGrid cell type must be default-constructible.");

	public:
		/**
		 * @brief Allocates and zero-initialises a @p InWidth x @p InHeight grid.
		 * @param InWidth    Column count. A non-positive value leaves the grid empty.
		 * @param InHeight   Row count. A non-positive value leaves the grid empty.
		 * @param InCellSize World-space size of one (square) cell.
		 * @param InOrigin   World position of the grid's lower-left corner, i.e. the
		 *                   corner of cell (0, 0).
		 */
		FTactixGrid(int32_t InWidth, int32_t InHeight, float InCellSize, FTactixVec2 InOrigin = {})
			: Width(InWidth)
			, Height(InHeight)
			, CellSize(InCellSize)
			, Origin(InOrigin)
			, Cells(InWidth > 0 && InHeight > 0 ? new T[static_cast<std::size_t>(InWidth) * static_cast<std::size_t>(InHeight)]{} : nullptr)
		{
		}

		/** @brief Frees the cell buffer. */
		~FTactixGrid() { delete[] Cells; }

		FTactixGrid(const FTactixGrid&)            = delete;
		FTactixGrid& operator=(const FTactixGrid&) = delete;

		/** @brief Steals @p Other's buffer and leaves it as a 0x0 grid. */
		FTactixGrid(FTactixGrid&& Other) noexcept
			: Width(Other.Width), Height(Other.Height), CellSize(Other.CellSize), Origin(Other.Origin), Cells(Other.Cells)
		{
			Other.Cells  = nullptr;
			Other.Width  = 0;
			Other.Height = 0;
		}

		/** @brief Frees this grid's buffer, then steals @p Other's. */
		FTactixGrid& operator=(FTactixGrid&& Other) noexcept
		{
			if (this != &Other)
			{
				delete[] Cells;
				Width        = Other.Width;
				Height       = Other.Height;
				CellSize     = Other.CellSize;
				Origin       = Other.Origin;
				Cells        = Other.Cells;
				Other.Cells  = nullptr;
				Other.Width  = 0;
				Other.Height = 0;
			}
			return *this;
		}

		/** @brief Column count. */
		TACTIX_NODISCARD int32_t     GetWidth()     const { return Width; }
		/** @brief Row count. */
		TACTIX_NODISCARD int32_t     GetHeight()    const { return Height; }
		/** @brief World-space size of one cell. */
		TACTIX_NODISCARD float       GetCellSize()  const { return CellSize; }
		/** @brief World position of cell (0, 0)'s corner. */
		TACTIX_NODISCARD FTactixVec2 GetOrigin()    const { return Origin; }
		/** @brief Total cell count, `Width * Height`. */
		TACTIX_NODISCARD std::size_t GetCellCount() const
		{
			return static_cast<std::size_t>(Width) * static_cast<std::size_t>(Height);
		}

		/**
		 * @brief Whether @p X, @p Y name a cell inside the grid.
		 * @return True if both coordinates are within `[0, Width)` / `[0, Height)`.
		 */
		TACTIX_NODISCARD bool InBounds(int32_t X, int32_t Y) const
		{
			return X >= 0 && Y >= 0 && X < Width && Y < Height;
		}

		/**
		 * @brief Flattens cell coordinates into a linear array index.
		 * @warning No bounds check. Pair with @ref InBounds when the coordinates
		 *          aren't already known good.
		 */
		TACTIX_NODISCARD std::size_t Index(int32_t X, int32_t Y) const
		{
			return static_cast<std::size_t>(Y) * static_cast<std::size_t>(Width) + static_cast<std::size_t>(X);
		}

		/**
		 * @brief Converts a world position to the cell that contains it.
		 * @param World World-space point.
		 * @param OutX  Receives the column.
		 * @param OutY  Receives the row.
		 * @note Uses @c std::floor, so points left of or below the origin map to
		 *       negative indices instead of rounding toward zero into cell 0.
		 *       The result can land outside the grid; check with @ref InBounds.
		 */
		void WorldToCell(FTactixVec2 World, int32_t& OutX, int32_t& OutY) const
		{
			OutX = static_cast<int32_t>(std::floor((World.X - Origin.X) / CellSize));
			OutY = static_cast<int32_t>(std::floor((World.Y - Origin.Y) / CellSize));
		}

		/**
		 * @brief World position of a cell's center.
		 * @param X Column.
		 * @param Y Row.
		 */
		TACTIX_NODISCARD FTactixVec2 CellCenterToWorld(int32_t X, int32_t Y) const
		{
			return { Origin.X + (static_cast<float>(X) + 0.5f) * CellSize,
			         Origin.Y + (static_cast<float>(Y) + 0.5f) * CellSize };
		}

		/** @brief Mutable cell access by coordinate. No bounds check. */
		TACTIX_NODISCARD T&       At(int32_t X, int32_t Y)       { return Cells[Index(X, Y)]; }
		/** @brief Const cell access by coordinate. No bounds check. */
		TACTIX_NODISCARD const T& At(int32_t X, int32_t Y) const { return Cells[Index(X, Y)]; }

		/** @brief Mutable cell access by flat index, e.g. from @ref Index. */
		TACTIX_NODISCARD T&       operator[](std::size_t Linear)       { return Cells[Linear]; }
		/** @brief Const cell access by flat index. */
		TACTIX_NODISCARD const T& operator[](std::size_t Linear) const { return Cells[Linear]; }

		/** @brief Raw pointer to the row-major buffer, for bulk passes. */
		TACTIX_NODISCARD T*       Data()       { return Cells; }
		/** @brief Const raw pointer to the row-major buffer. */
		TACTIX_NODISCARD const T* Data() const { return Cells; }

		/** @brief Sets every cell to @p Value. */
		void Fill(const T& Value)
		{
			const std::size_t N = GetCellCount();
			for (std::size_t i = 0; i < N; ++i) Cells[i] = Value;
		}

		/**
		 * @brief Gathers the linear indices of a cell's in-bounds neighbours.
		 * @param X          Column of the center cell.
		 * @param Y          Row of the center cell.
		 * @param OutIndices Destination for up to 8 neighbour indices (8-connected).
		 * @return How many indices were written (fewer than 8 at edges and corners).
		 */
		int32_t GetNeighbours(int32_t X, int32_t Y, std::size_t OutIndices[8]) const
		{
			static constexpr int32_t DX[8] = { -1,  0,  1, -1, 1, -1, 0, 1 };
			static constexpr int32_t DY[8] = { -1, -1, -1,  0, 0,  1, 1, 1 };
			int32_t N = 0;
			for (int32_t i = 0; i < 8; ++i)
			{
				const int32_t NX = X + DX[i];
				const int32_t NY = Y + DY[i];
				if (InBounds(NX, NY))
				{
					OutIndices[N++] = Index(NX, NY);
				}
			}
			return N;
		}

	private:
		int32_t     Width{0};       ///< Column count.
		int32_t     Height{0};      ///< Row count.
		float       CellSize{1.0f}; ///< World size of one square cell.
		FTactixVec2 Origin{};       ///< World corner of cell (0, 0).
		T*          Cells{nullptr}; ///< Row-major cell buffer, allocated once.
	};
}
