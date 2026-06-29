// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixCoverSystem.h
 * @brief Registry of cover points with claim/release ownership and a "best cover
 *        for this situation" query.
 */

#pragma once

#include "TactixApi.h"
#include "Cover/FTactixCoverPoint.h"
#include "Foundation/TactixPool.h"
#include "Foundation/TactixHandle.h"
#include "Foundation/TactixMath.h"

#include <cstddef>
#include <cstdint>

namespace Tactix
{
	/**
	 * @brief Holds the level's cover points and answers tactical queries over them.
	 *
	 * Backed by a generational @ref FTactixPool, so cover handles stay safe across
	 * frames even as points are registered and removed. The query is a linear scan,
	 * which is deliberate: at a few hundred points per level it's faster and simpler
	 * than a spatial index. Profile before raising @p Capacity into open-world
	 * territory and reaching for something fancier.
	 *
	 * @tparam Capacity Maximum simultaneous cover points. Default 512.
	 */
	template <std::size_t Capacity = 512>
	class FTactixCoverSystem
	{
	public:
		/** @brief Handle to a registered cover point. */
		using FHandle    = FTactixHandle<FTactixCoverPoint>;
		/** @brief Handle to an agent (claimer/querier). */
		using FAgentHandle = FTactixHandle<ITactixAgent>;

		/** @brief Parameters describing what kind of cover an agent is looking for. */
		struct FQuery
		{
			FTactixVec3  FromPosition{};    ///< Where the agent is now; nearer cover scores higher.
			FTactixVec3  ThreatPosition{};  ///< Where the danger is; cover must face away from it.
			float        MaxRange{2000.0f}; ///< Cover beyond this distance is rejected.

			/**
			 * @brief Minimum blocking quality, as `-dot(threatDir, normal)`.
			 *
			 * 0 accepts any point that blocks the threat at all; higher values demand
			 * the cover face the threat more squarely (around 0.5 is a ~60 degree
			 * acceptance wedge).
			 */
			float        MinBlockDot{0.0f};

			bool         bExcludeClaimed{true}; ///< Skip points already claimed by someone else.
			FAgentHandle Querier{};             ///< The asking agent; lets it re-pick a point it already owns.
		};

		/** @brief Registers a cover point and returns a handle to it. */
		FHandle Register(const FTactixCoverPoint& P)      { return Pool.Alloc(P); }
		/** @brief Removes a cover point. Returns false if the handle is stale. */
		bool    Unregister(FHandle H)                     { return Pool.Free(H); }

		/** @brief Const access to a registered point, or @c nullptr if stale. */
		const FTactixCoverPoint* Get(FHandle H) const     { return Pool.Resolve(H); }
		/** @brief Mutable access to a registered point, or @c nullptr if stale. */
		FTactixCoverPoint*       Get(FHandle H)           { return Pool.Resolve(H); }

		/** @brief Number of registered cover points. */
		std::size_t Num() const                           { return Pool.Num(); }

		/**
		 * @brief Reserves a cover point for an agent.
		 * @param H     Point to claim.
		 * @param Agent Claiming agent.
		 * @return True on success. Fails on a stale handle, an invalid agent, or a
		 *         point already claimed by someone else. Re-claiming a point you
		 *         already own succeeds.
		 */
		bool Claim(FHandle H, FAgentHandle Agent)
		{
			FTactixCoverPoint* P = Pool.Resolve(H);
			if (P == nullptr || !Agent.IsValid())           return false;
			if (P->ClaimedBy.IsValid() && P->ClaimedBy != Agent) return false;
			P->ClaimedBy = Agent;
			return true;
		}

		/**
		 * @brief Releases a claim.
		 * @param H     Point to release.
		 * @param Agent The agent that holds it.
		 * @return True if @p Agent held @p H and it was released; false otherwise.
		 *         Releasing a point you don't own is rejected, so one agent can't
		 *         free another's cover.
		 */
		bool Release(FHandle H, FAgentHandle Agent)
		{
			FTactixCoverPoint* P = Pool.Resolve(H);
			if (P == nullptr) return false;
			if (P->ClaimedBy != Agent) return false;
			P->ClaimedBy = FAgentHandle::Invalid();
			return true;
		}

		/**
		 * @brief Visits every live cover point.
		 * @tparam Fn Callable `void(FHandle, FTactixCoverPoint&)`.
		 */
		template <typename Fn>
		void ForEach(Fn&& Func) { Pool.ForEach(std::forward<Fn>(Func)); }

		/**
		 * @brief Finds the best cover point for a query.
		 * @param Q The situation: where the agent is, where the threat is, limits.
		 * @return Handle to the highest-scoring point, or an invalid handle if none
		 *         qualifies. See @ref ScoreOne for how candidates are ranked.
		 */
		FHandle QueryBest(const FQuery& Q) const
		{
			FHandle Best{};
			float   BestScore = 0.0f;

			Pool.ForEach([&](FHandle H, const FTactixCoverPoint& P)
			{
				const float Score = ScoreOne(P, Q);
				if (Score > BestScore)
				{
					BestScore = Score;
					Best      = H;
				}
			});
			return Best;
		}

	private:
		mutable FTactixPool<FTactixCoverPoint, Capacity> Pool; ///< Backing storage; mutable so const queries can still iterate.

		/**
		 * @brief Scores one cover point against a query.
		 *
		 * Returns 0 (disqualified) for a claimed point the querier doesn't own when
		 * exclusion is on, a degenerate threat distance, blocking below
		 * @c MinBlockDot, or a point past @c MaxRange. Otherwise the score combines
		 * three factors: how squarely it faces the threat, how close it is (linear
		 * falloff to @c MaxRange), and a slight bias toward high cover.
		 *
		 * @return A score > 0 for a valid candidate, 0 to reject.
		 */
		static float ScoreOne(const FTactixCoverPoint& P, const FQuery& Q)
		{
			if (P.IsClaimed() && Q.bExcludeClaimed && P.ClaimedBy != Q.Querier) return 0.0f;

			const FTactixVec3 ToThreat = Q.ThreatPosition - P.Position;
			const float ThreatDistSq = ToThreat.LengthSquared();
			if (ThreatDistSq < 1e-4f) return 0.0f;

			const float       ThreatDist = std::sqrt(ThreatDistSq);
			const FTactixVec3 ThreatDir  = ToThreat / ThreatDist;

			const float Block = -ThreatDir.Dot(P.Normal);
			if (Block < Q.MinBlockDot) return 0.0f;

			const FTactixVec3 ToPoint = P.Position - Q.FromPosition;
			const float Range = ToPoint.Length();
			if (Range > Q.MaxRange) return 0.0f;

			const float RangeFactor = (Q.MaxRange > 0.0f) ? (1.0f - (Range / Q.MaxRange)) : 1.0f;
			const float BlockNorm   = 0.5f * (Block + 1.0f);
			const float HeightBias  = (P.Height == ETactixCoverHeight::High) ? 1.0f : 0.9f;

			return BlockNorm * RangeFactor * HeightBias;
		}
	};
}
