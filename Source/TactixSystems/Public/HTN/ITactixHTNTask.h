// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "GOAP/FTactixWorldState.h"

#include <cstdint>

namespace Tactix
{
	struct FTactixAgentContext;

	class TACTIXSYSTEMS_API ITactixHTNTask
	{
	public:
		virtual ~ITactixHTNTask() = default;

		// True for primitives (leaves), false for compounds (methods).
		virtual bool IsPrimitive() const = 0;
	};

	class TACTIXSYSTEMS_API ITactixHTNPrimitive : public ITactixHTNTask
	{
	public:
		virtual bool IsApplicable(const FTactixWorldState& State,
		                          const FTactixAgentContext& Ctx) const = 0;

		// Planner-visible effects. Actual runtime Execute may differ in nuance
		// but must keep these bits honest or the plan won't match reality.
		virtual FTactixWorldState GetEffects(const FTactixAgentContext& Ctx) const = 0;

		virtual void Execute(FTactixAgentContext& Ctx) = 0;
	};

	class TACTIXSYSTEMS_API ITactixHTNCompound : public ITactixHTNTask
	{
	public:
		virtual uint32_t GetMethodCount() const = 0;

		virtual bool MethodApplies(uint32_t MethodIndex,
		                           const FTactixWorldState& State,
		                           const FTactixAgentContext& Ctx) const = 0;

		// Fill OutSubtasks (up to MaxCount) and return written count. Planner
		// tries methods in index order; return the preferred method first.
		virtual uint32_t GetMethodSubtasks(uint32_t          MethodIndex,
		                                   ITactixHTNTask**  OutSubtasks,
		                                   uint32_t          MaxCount) const = 0;
	};
}
