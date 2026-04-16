// Copyright Sleak Software. All Rights Reserved.
//
// ITactixPerceptionChannel + FTactixStimulus — base types for the perception
// memory subsystem.
//
// A "channel" represents a kind of stimulus (sight, sound, damage, radio,
// smell). Projects can add more by implementing ITactixPerceptionChannel and
// assigning a unique `ChannelId`. The perception memory treats channels as
// opaque IDs with per-channel decay half-lives.

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Foundation/TactixMath.h"
#include "Agent/ITactixAgent.h"

#include <cstdint>

namespace Tactix
{
	enum class ETactixStimulusKind : uint8_t
	{
		Unknown = 0,
		Sight,
		Sound,
		Damage,
		Radio,
		Smell,
		Touch,
		UserDefined,
	};

	// 32-byte stimulus record — sized to keep a ring buffer of 32 entries
	// under 1 KiB per agent.
	struct FTactixStimulus
	{
		FTactixVec3                  Origin{};                                // world position of event
		double                       Timestamp{0.0};                           // seconds (game time)
		float                        Intensity{1.0f};                          // [0, 1] at emit, decays over time
		uint16_t                     ChannelId{0};                             // channel classification
		ETactixStimulusKind          Kind{ETactixStimulusKind::Unknown};       // coarse category
		uint8_t                      _Pad{0};
		FTactixHandle<ITactixAgent>  Source{};                                 // who caused it (may be invalid)
	};

	class TACTIXCORE_API ITactixPerceptionChannel
	{
	public:
		virtual ~ITactixPerceptionChannel() = default;

		// Unique channel ID. Values 0–31 are reserved for Tactix-defined
		// channels; user channels should start at 32.
		virtual uint16_t GetChannelId() const = 0;

		// Seconds over which an intensity of 1.0 should decay to 0.5. The
		// perception memory multiplies stimuli by 2^(-age/half_life).
		virtual float GetDecayHalfLife() const = 0;
	};
}
