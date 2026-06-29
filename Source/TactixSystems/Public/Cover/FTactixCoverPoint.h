// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixCoverPoint.h
 * @brief A single piece of cover: where to stand, which way it faces, and who
 *        currently owns it.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Foundation/TactixMath.h"
#include "Agent/ITactixAgent.h"

#include <cstdint>

namespace Tactix
{
	/** @brief How tall a cover point is, which affects how it scores and is used. */
	enum class ETactixCoverHeight : uint8_t
	{
		Low,   ///< Crouch cover; lean/pop out to fire.
		High,  ///< Full-height cover; fire around an edge.
	};

	/**
	 * @brief One cover position registered with @ref FTactixCoverSystem.
	 *
	 * @c Normal points away from the wall into the open space the agent is exposed
	 * to. A threat counts as blocked when it sits roughly along @c -Normal from
	 * @c Position, i.e. on the wall side. @c ClaimedBy records the agent that has
	 * reserved this point, if any, so two agents don't pile into the same spot.
	 */
	struct FTactixCoverPoint
	{
		FTactixVec3                 Position{};                      ///< Where the agent stands or crouches.
		FTactixVec3                 Normal{1.0f, 0.0f, 0.0f};        ///< Unit vector away from the wall, toward the exposed side.
		ETactixCoverHeight          Height{ETactixCoverHeight::Low}; ///< Low or high cover.
		FTactixHandle<ITactixAgent> ClaimedBy{};                     ///< Owner agent, or invalid if unclaimed.

		/** @brief Whether some agent currently holds this point. */
		bool IsClaimed() const { return ClaimedBy.IsValid(); }
	};
}
