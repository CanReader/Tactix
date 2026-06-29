// Copyright Sleak Software. All Rights Reserved.

/**
 * @file ITactixPerceptionChannel.h
 * @brief Stimulus record and channel interface for the perception memory.
 *
 * A "channel" is a kind of stimulus: sight, sound, damage, radio, smell, and so
 * on. The perception memory itself doesn't care what a channel means; it just
 * stores stimuli tagged with a channel ID and fades them according to that
 * channel's half-life. Projects extend the set by implementing
 * @ref Tactix::ITactixPerceptionChannel "ITactixPerceptionChannel" with a fresh @c ChannelId and half-life.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Foundation/TactixMath.h"
#include "Agent/ITactixAgent.h"

#include <cstdint>

namespace Tactix
{
	/** @brief Coarse category for a stimulus, orthogonal to its numeric channel ID. */
	enum class ETactixStimulusKind : uint8_t
	{
		Unknown = 0,  ///< Unclassified.
		Sight,        ///< Saw something.
		Sound,        ///< Heard something.
		Damage,       ///< Took damage.
		Radio,        ///< Received a comms/radio cue.
		Smell,        ///< Scent trail.
		Touch,        ///< Physical contact.
		UserDefined,  ///< Project-specific category.
	};

	/**
	 * @brief A single timestamped sensory event.
	 *
	 * Laid out to stay compact (one cache-line-ish record) so an agent can keep a
	 * ring of them cheaply. @c _Pad exists only to make that layout explicit.
	 */
	struct FTactixStimulus
	{
		FTactixVec3                  Origin{};                            ///< World position the event came from.
		double                       Timestamp{0.0};                      ///< Game time (seconds) the event was recorded.
		float                        Intensity{1.0f};                     ///< Strength in [0, 1] at emit; fades with age.
		uint16_t                     ChannelId{0};                        ///< Which channel classified this stimulus.
		ETactixStimulusKind          Kind{ETactixStimulusKind::Unknown};  ///< Coarse category.
		uint8_t                      _Pad{0};                             ///< Explicit padding; keeps the layout stable.
		FTactixHandle<ITactixAgent>  Source{};                            ///< Agent that caused it; may be invalid (e.g. an environmental sound).
	};

	/**
	 * @brief Describes one perception channel: its ID and how fast it fades.
	 *
	 * Implement this per stimulus type. The memory consults @ref GetDecayHalfLife
	 * to age stored stimuli of this channel.
	 */
	class TACTIXCORE_API ITactixPerceptionChannel
	{
	public:
		virtual ~ITactixPerceptionChannel() = default;

		/**
		 * @brief This channel's unique numeric ID.
		 * @return The ID. IDs 0 to 31 are reserved for Tactix's own channels;
		 *         project channels should start at 32 to avoid collisions.
		 */
		virtual uint16_t GetChannelId() const = 0;

		/**
		 * @brief Half-life of this channel's stimuli, in seconds.
		 * @return The time over which an intensity of 1.0 fades to 0.5. The memory
		 *         scales intensity by `2^(-age / half_life)`, so a larger value
		 *         means the stimulus lingers longer.
		 */
		virtual float GetDecayHalfLife() const = 0;
	};
}
