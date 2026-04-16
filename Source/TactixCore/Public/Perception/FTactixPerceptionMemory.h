// Copyright Sleak Software. All Rights Reserved.
//
// FTactixPerceptionMemory — per-agent ring buffer of timestamped stimuli.
//
// The ring buffer is sized to a compile-time power-of-2 so "most recent N"
// iteration uses a cheap bitmask instead of a modulo divide. Decay is applied
// on demand: the caller supplies the current time and a half-life (usually
// from the channel that produced the stimulus). Stimuli whose decayed
// intensity falls below a floor are zeroed in place but left in the buffer —
// the ring will eventually overwrite them naturally, so explicit eviction is
// unnecessary.

#pragma once

#include "TactixApi.h"
#include "Perception/ITactixPerceptionChannel.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Tactix
{
	template <std::size_t Capacity = 32>
	class FTactixPerceptionMemory
	{
		static_assert(Capacity > 0,                           "FTactixPerceptionMemory capacity must be > 0.");
		static_assert((Capacity & (Capacity - 1)) == 0,       "FTactixPerceptionMemory capacity must be a power of 2.");

	public:
		static constexpr std::size_t kCapacity = Capacity;

		FTactixPerceptionMemory() = default;

		void Insert(const FTactixStimulus& Stim)
		{
			Buf[Head & Mask] = Stim;
			++Head;
			if (Size < Capacity) ++Size;
		}

		// Apply half-life decay to every stored stimulus and zero out any
		// whose intensity falls below `Floor`. The default half-life is used
		// when no channel lookup is available.
		void Decay(double NowSeconds, float DefaultHalfLife = 5.0f, float Floor = 0.05f)
		{
			const float SafeHalfLife = DefaultHalfLife > 0.0f ? DefaultHalfLife : 1.0f;
			for (std::size_t i = 0; i < Size; ++i)
			{
				FTactixStimulus& S = PeekMutable(i);
				if (S.Intensity <= 0.0f) continue;
				const float Age = static_cast<float>(NowSeconds - S.Timestamp);
				if (Age <= 0.0f) continue;
				const float Decayed = S.Intensity * std::exp2(-Age / SafeHalfLife);
				S.Intensity = Decayed < Floor ? 0.0f : Decayed;
			}
		}

		// Copy up to `OutCapacity` live stimuli for `ChannelId` into `Out`,
		// most-recent first. Returns the number written.
		std::size_t Recent(uint16_t ChannelId, FTactixStimulus* Out, std::size_t OutCapacity) const
		{
			if (Out == nullptr || OutCapacity == 0) return 0;
			std::size_t N = 0;
			for (std::size_t i = 0; i < Size && N < OutCapacity; ++i)
			{
				const FTactixStimulus& S = Peek(i);
				if (S.ChannelId == ChannelId && S.Intensity > 0.0f)
				{
					Out[N++] = S;
				}
			}
			return N;
		}

		// Return the strongest live stimulus for `ChannelId`, or nullptr if none.
		TACTIX_NODISCARD const FTactixStimulus* StrongestForChannel(uint16_t ChannelId) const
		{
			const FTactixStimulus* Best = nullptr;
			for (std::size_t i = 0; i < Size; ++i)
			{
				const FTactixStimulus& S = Peek(i);
				if (S.ChannelId != ChannelId || S.Intensity <= 0.0f) continue;
				if (Best == nullptr || S.Intensity > Best->Intensity) Best = &S;
			}
			return Best;
		}

		// Return the most recent (highest Timestamp) live stimulus for `ChannelId`.
		TACTIX_NODISCARD const FTactixStimulus* LatestForChannel(uint16_t ChannelId) const
		{
			// i==0 is newest in Peek order, so the first match IS the most recent.
			for (std::size_t i = 0; i < Size; ++i)
			{
				const FTactixStimulus& S = Peek(i);
				if (S.ChannelId == ChannelId && S.Intensity > 0.0f) return &S;
			}
			return nullptr;
		}

		TACTIX_NODISCARD std::size_t Num() const { return Size; }
		void Clear() { Head = 0; Size = 0; }

	private:
		static constexpr std::size_t Mask = Capacity - 1;
		FTactixStimulus Buf[Capacity]{};
		std::size_t     Head{0};
		std::size_t     Size{0};

		TACTIX_FORCEINLINE FTactixStimulus& PeekMutable(std::size_t i)
		{
			// i == 0 is newest; walk backwards from Head.
			return Buf[(Head - 1 - i) & Mask];
		}
		TACTIX_FORCEINLINE const FTactixStimulus& Peek(std::size_t i) const
		{
			return Buf[(Head - 1 - i) & Mask];
		}
	};
}
