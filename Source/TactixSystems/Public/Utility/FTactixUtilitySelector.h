// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "Utility/ITactixAction.h"
#include "Utility/ITactixConsideration.h"

#include <cstddef>

namespace Tactix
{
	struct FTactixAgentContext;

	struct FTactixUtilityResult
	{
		static constexpr std::size_t kNoWinner = ~std::size_t{0};

		std::size_t    Index{kNoWinner};
		float          Score{0.0f};
		ITactixAction* Action{nullptr};

		bool HasWinner() const { return Index != kNoWinner && Action != nullptr; }
	};

	struct FTactixUtilitySelector
	{
		// Ties go to the lower index so selection is stable frame-to-frame.
		static FTactixUtilityResult Pick(ITactixAction* const* Actions, std::size_t Count,
		                                 const FTactixAgentContext& Ctx)
		{
			FTactixUtilityResult Best{};

			for (std::size_t i = 0; i < Count; ++i)
			{
				ITactixAction* A = Actions[i];
				if (A == nullptr || !A->IsValid(Ctx)) continue;

				const float Score = A->GetScore(Ctx);
				if (Score > Best.Score)
				{
					Best.Index  = i;
					Best.Score  = Score;
					Best.Action = A;
				}
			}

			return Best;
		}

		// Mike Lewis compensation. Without it, each extra consideration drags
		// the product toward zero and wider actions lose unfairly.
		static float ScoreConsiderations(const ITactixConsideration* const* Considerations,
		                                 std::size_t Count, const FTactixAgentContext& Ctx)
		{
			if (Count == 0 || Considerations == nullptr) return 0.0f;

			const float ModFactor = 1.0f - (1.0f / static_cast<float>(Count));
			float Product = 1.0f;

			for (std::size_t i = 0; i < Count; ++i)
			{
				const ITactixConsideration* C = Considerations[i];
				if (C == nullptr) return 0.0f;

				float X = C->Evaluate(Ctx);
				if (X <= 0.0f) return 0.0f;
				if (X > 1.0f)  X = 1.0f;

				const float MakeUp = (1.0f - X) * ModFactor;
				X = X + (MakeUp * X);

				Product *= X;
			}

			return Product;
		}
	};
}
