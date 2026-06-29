// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixPerceptionMemory.h
 * @brief Per-agent ring buffer of timestamped stimuli with on-demand decay.
 *
 * Each agent keeps one of these as its short-term sensory memory. Capacity is a
 * compile-time power of two so that walking "the most recent N" wraps with a
 * bitmask instead of a modulo. Decay isn't a background process: the owner calls
 * @ref Tactix::FTactixPerceptionMemory::Decay "Decay" with the current time and a half-life
 * (usually the producing channel's). When a stimulus fades below the floor its
 * intensity is zeroed in place but the slot is left alone; the ring overwrites it
 * in due course, so there's no separate eviction step.
 *
 * @tparam Capacity Number of stimuli retained. Must be a power of two.
 */

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
		/** @brief Compile-time stimulus capacity. */
		static constexpr std::size_t kCapacity = Capacity;

		FTactixPerceptionMemory() = default;

		/**
		 * @brief Records a stimulus, overwriting the oldest once the ring is full.
		 * @param Stim The stimulus to store. It becomes the new "most recent".
		 */
		void Insert(const FTactixStimulus& Stim)
		{
			Buf[Head & Mask] = Stim;
			++Head;
			if (Size < Capacity) ++Size;
		}

		/**
		 * @brief Ages every stored stimulus by half-life decay.
		 *
		 * Scales each stimulus by `2^(-age / half_life)` and zeroes any that drop
		 * below @p Floor. Stimuli already at zero, or whose timestamp is in the
		 * future relative to @p NowSeconds, are skipped.
		 *
		 * @param NowSeconds     Current game time in seconds.
		 * @param DefaultHalfLife Half-life applied to all stimuli here, in seconds.
		 *        This call decays uniformly; per-channel half-lives are applied by
		 *        decaying with the relevant channel's value. A non-positive value is
		 *        clamped up to 1 to avoid dividing by zero.
		 * @param Floor          Intensity below which a stimulus is treated as gone.
		 */
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

		/**
		 * @brief Copies the live stimuli for one channel, newest first.
		 * @param ChannelId   Channel to filter on.
		 * @param Out         Caller buffer to receive matching stimuli.
		 * @param OutCapacity Size of @p Out.
		 * @return Count written, at most @p OutCapacity. Faded (zero-intensity)
		 *         stimuli are excluded.
		 */
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

		/**
		 * @brief Highest-intensity live stimulus on a channel.
		 * @param ChannelId Channel to search.
		 * @return Pointer to the strongest matching stimulus, or @c nullptr if none
		 *         are live. The pointer is into the ring; it's invalidated by the
		 *         next @ref Insert.
		 */
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

		/**
		 * @brief Most recently recorded live stimulus on a channel.
		 * @param ChannelId Channel to search.
		 * @return Pointer to the newest matching stimulus, or @c nullptr if none are
		 *         live. Invalidated by the next @ref Insert.
		 */
		TACTIX_NODISCARD const FTactixStimulus* LatestForChannel(uint16_t ChannelId) const
		{
			// Peek order runs newest-first, so the first match is the most recent.
			for (std::size_t i = 0; i < Size; ++i)
			{
				const FTactixStimulus& S = Peek(i);
				if (S.ChannelId == ChannelId && S.Intensity > 0.0f) return &S;
			}
			return nullptr;
		}

		/** @brief Number of stimuli currently held (including faded-but-not-overwritten). */
		TACTIX_NODISCARD std::size_t Num() const { return Size; }
		/** @brief Forgets everything. */
		void Clear() { Head = 0; Size = 0; }

	private:
		static constexpr std::size_t Mask = Capacity - 1; ///< Wrap mask; valid because capacity is a power of two.
		FTactixStimulus Buf[Capacity]{};                  ///< Backing ring storage.
		std::size_t     Head{0};                          ///< Write cursor; one past the newest entry.
		std::size_t     Size{0};                          ///< Live entry count, capped at @p Capacity.

		/** @brief Mutable access by age, @p i == 0 being newest, walking back from @c Head. */
		TACTIX_FORCEINLINE FTactixStimulus& PeekMutable(std::size_t i)
		{
			return Buf[(Head - 1 - i) & Mask];
		}
		/** @brief Const access by age, @p i == 0 being newest. */
		TACTIX_FORCEINLINE const FTactixStimulus& Peek(std::size_t i) const
		{
			return Buf[(Head - 1 - i) & Mask];
		}
	};
}
