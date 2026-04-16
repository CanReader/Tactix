// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixMath.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Tactix
{
	// Multi-channel 2D scalar grid. Channels are laid out back-to-back in a
	// single heap allocation so per-channel passes (decay, blur) scan
	// contiguous memory.
	template <std::size_t ChannelCount>
	class FTactixInfluenceMap
	{
		static_assert(ChannelCount > 0, "FTactixInfluenceMap needs at least one channel.");

	public:
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

		~FTactixInfluenceMap() { delete[] Data; }

		FTactixInfluenceMap(const FTactixInfluenceMap&)            = delete;
		FTactixInfluenceMap& operator=(const FTactixInfluenceMap&) = delete;

		FTactixInfluenceMap(FTactixInfluenceMap&& O) noexcept
			: Width(O.Width), Height(O.Height), CellSize(O.CellSize), Origin(O.Origin), Data(O.Data)
		{
			O.Data   = nullptr;
			O.Width  = 0;
			O.Height = 0;
		}

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

		int32_t     GetWidth()    const { return Width; }
		int32_t     GetHeight()   const { return Height; }
		float       GetCellSize() const { return CellSize; }
		FTactixVec2 GetOrigin()   const { return Origin; }
		std::size_t NumCells()    const { return static_cast<std::size_t>(Width) * static_cast<std::size_t>(Height); }

		// Linear falloff stamp: contribution = Value * max(0, 1 - d/Radius). Stamps
		// accumulate; call Decay or Clear to drain them.
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

		// Rate in [0, 1]: cell *= (1 - Rate). 0 keeps, 1 clears.
		void Decay(std::size_t Channel, float Rate)
		{
			if (Channel >= ChannelCount || Data == nullptr) return;
			const float Factor = 1.0f - Clamp(Rate, 0.0f, 1.0f);
			float* Ch = ChannelData(Channel);
			const std::size_t N = NumCells();
			for (std::size_t i = 0; i < N; ++i) Ch[i] *= Factor;
		}

		void Clear(std::size_t Channel)
		{
			if (Channel >= ChannelCount || Data == nullptr) return;
			float* Ch = ChannelData(Channel);
			const std::size_t N = NumCells();
			for (std::size_t i = 0; i < N; ++i) Ch[i] = 0.0f;
		}

		float SampleNearest(std::size_t Channel, FTactixVec2 World) const
		{
			if (Channel >= ChannelCount || Data == nullptr) return 0.0f;
			int32_t X, Y;
			WorldToCell(World, X, Y);
			if (X < 0 || Y < 0 || X >= Width || Y >= Height) return 0.0f;
			return ChannelData(Channel)[Idx(X, Y)];
		}

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

		const float* ChannelData(std::size_t Channel) const { return Data + Channel * NumCells(); }
		float*       ChannelData(std::size_t Channel)       { return Data + Channel * NumCells(); }

		void WorldToCell(FTactixVec2 World, int32_t& OutX, int32_t& OutY) const
		{
			OutX = static_cast<int32_t>(std::floor((World.X - Origin.X) / CellSize));
			OutY = static_cast<int32_t>(std::floor((World.Y - Origin.Y) / CellSize));
		}

		FTactixVec2 CellCenter(int32_t X, int32_t Y) const
		{
			return { Origin.X + (static_cast<float>(X) + 0.5f) * CellSize,
			         Origin.Y + (static_cast<float>(Y) + 0.5f) * CellSize };
		}

	private:
		int32_t     Width{0};
		int32_t     Height{0};
		float       CellSize{1.0f};
		FTactixVec2 Origin{};
		float*      Data{nullptr};

		std::size_t Idx(int32_t X, int32_t Y) const
		{
			return static_cast<std::size_t>(Y) * static_cast<std::size_t>(Width) + static_cast<std::size_t>(X);
		}
	};
}
