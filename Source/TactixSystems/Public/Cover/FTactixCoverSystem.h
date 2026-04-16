// Copyright Sleak Software. All Rights Reserved.

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
	// Linear-scan registry backed by a generational pool. 512 default capacity
	// is plenty for typical shooter levels; bump the template arg for open-world
	// streaming if needed and profile before reaching for a spatial index.
	template <std::size_t Capacity = 512>
	class FTactixCoverSystem
	{
	public:
		using FHandle    = FTactixHandle<FTactixCoverPoint>;
		using FAgentHandle = FTactixHandle<ITactixAgent>;

		struct FQuery
		{
			FTactixVec3  FromPosition{};
			FTactixVec3  ThreatPosition{};
			float        MaxRange{2000.0f};

			// Minimum required -dot(threatDir, normal) for a point to count as
			// cover. 0 = any angle that blocks at all. 0.5 = ~60deg wedge.
			float        MinBlockDot{0.0f};

			bool         bExcludeClaimed{true};
			FAgentHandle Querier{};   // lets Querier pick their own already-claimed point
		};

		FHandle Register(const FTactixCoverPoint& P)      { return Pool.Alloc(P); }
		bool    Unregister(FHandle H)                     { return Pool.Free(H); }

		const FTactixCoverPoint* Get(FHandle H) const     { return Pool.Resolve(H); }
		FTactixCoverPoint*       Get(FHandle H)           { return Pool.Resolve(H); }

		std::size_t Num() const                           { return Pool.Num(); }

		bool Claim(FHandle H, FAgentHandle Agent)
		{
			FTactixCoverPoint* P = Pool.Resolve(H);
			if (P == nullptr || !Agent.IsValid())           return false;
			if (P->ClaimedBy.IsValid() && P->ClaimedBy != Agent) return false;
			P->ClaimedBy = Agent;
			return true;
		}

		bool Release(FHandle H, FAgentHandle Agent)
		{
			FTactixCoverPoint* P = Pool.Resolve(H);
			if (P == nullptr) return false;
			if (P->ClaimedBy != Agent) return false;
			P->ClaimedBy = FAgentHandle::Invalid();
			return true;
		}

		// Walk all live cover points. Functor: void(FHandle, FTactixCoverPoint&).
		template <typename Fn>
		void ForEach(Fn&& Func) { Pool.ForEach(std::forward<Fn>(Func)); }

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
		mutable FTactixPool<FTactixCoverPoint, Capacity> Pool;

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
