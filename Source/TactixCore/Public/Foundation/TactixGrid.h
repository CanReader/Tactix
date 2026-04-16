// Copyright Sleak Software. All Rights Reserved.
//
// TactixGrid — engine-agnostic 2D grid with world<->cell coordinate helpers.
//
// Storage is a flat row-major array of T so the influence map and other L3
// systems can stamp / blur with a tight inner loop. Allocation is done once
// at construction (not a hot path) — subsequent reads and writes touch only
// the existing buffer.

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
		FTactixGrid(int32_t InWidth, int32_t InHeight, float InCellSize, FTactixVec2 InOrigin = {})
			: Width(InWidth)
			, Height(InHeight)
			, CellSize(InCellSize)
			, Origin(InOrigin)
			, Cells(InWidth > 0 && InHeight > 0 ? new T[static_cast<std::size_t>(InWidth) * static_cast<std::size_t>(InHeight)]{} : nullptr)
		{
		}

		~FTactixGrid() { delete[] Cells; }

		FTactixGrid(const FTactixGrid&)            = delete;
		FTactixGrid& operator=(const FTactixGrid&) = delete;

		FTactixGrid(FTactixGrid&& Other) noexcept
			: Width(Other.Width), Height(Other.Height), CellSize(Other.CellSize), Origin(Other.Origin), Cells(Other.Cells)
		{
			Other.Cells  = nullptr;
			Other.Width  = 0;
			Other.Height = 0;
		}

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

		// ---- Query -----------------------------------------------------------

		TACTIX_NODISCARD int32_t     GetWidth()     const { return Width; }
		TACTIX_NODISCARD int32_t     GetHeight()    const { return Height; }
		TACTIX_NODISCARD float       GetCellSize()  const { return CellSize; }
		TACTIX_NODISCARD FTactixVec2 GetOrigin()    const { return Origin; }
		TACTIX_NODISCARD std::size_t GetCellCount() const
		{
			return static_cast<std::size_t>(Width) * static_cast<std::size_t>(Height);
		}

		TACTIX_NODISCARD bool InBounds(int32_t X, int32_t Y) const
		{
			return X >= 0 && Y >= 0 && X < Width && Y < Height;
		}

		TACTIX_NODISCARD std::size_t Index(int32_t X, int32_t Y) const
		{
			return static_cast<std::size_t>(Y) * static_cast<std::size_t>(Width) + static_cast<std::size_t>(X);
		}

		// ---- World <-> cell -------------------------------------------------

		void WorldToCell(FTactixVec2 World, int32_t& OutX, int32_t& OutY) const
		{
			// std::floor keeps negative coordinates on the correct side of the grid.
			OutX = static_cast<int32_t>(std::floor((World.X - Origin.X) / CellSize));
			OutY = static_cast<int32_t>(std::floor((World.Y - Origin.Y) / CellSize));
		}

		TACTIX_NODISCARD FTactixVec2 CellCenterToWorld(int32_t X, int32_t Y) const
		{
			return { Origin.X + (static_cast<float>(X) + 0.5f) * CellSize,
			         Origin.Y + (static_cast<float>(Y) + 0.5f) * CellSize };
		}

		// ---- Access ---------------------------------------------------------

		TACTIX_NODISCARD T&       At(int32_t X, int32_t Y)       { return Cells[Index(X, Y)]; }
		TACTIX_NODISCARD const T& At(int32_t X, int32_t Y) const { return Cells[Index(X, Y)]; }

		TACTIX_NODISCARD T&       operator[](std::size_t Linear)       { return Cells[Linear]; }
		TACTIX_NODISCARD const T& operator[](std::size_t Linear) const { return Cells[Linear]; }

		TACTIX_NODISCARD T*       Data()       { return Cells; }
		TACTIX_NODISCARD const T* Data() const { return Cells; }

		void Fill(const T& Value)
		{
			const std::size_t N = GetCellCount();
			for (std::size_t i = 0; i < N; ++i) Cells[i] = Value;
		}

		// Up-to-8-connected neighbour indices. Returns the count written.
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
		int32_t     Width{0};
		int32_t     Height{0};
		float       CellSize{1.0f};
		FTactixVec2 Origin{};
		T*          Cells{nullptr};
	};
}
