// Copyright Sleak Software. All Rights Reserved.

#include "Agent/FTactixAgentScheduler.h"

#include <cstring>  // std::memcpy
#include <utility>  // std::swap

namespace Tactix
{
	FTactixAgentScheduler::FTactixAgentScheduler(std::size_t InitialCapacity)
	{
		if (InitialCapacity == 0) InitialCapacity = 16;
		Grow(InitialCapacity);
	}

	FTactixAgentScheduler::~FTactixAgentScheduler()
	{
		delete[] Heap;
		Heap = nullptr;
		Count = 0;
		Cap = 0;
	}

	void FTactixAgentScheduler::Grow(std::size_t NewCapacity)
	{
		if (NewCapacity <= Cap) return;
		auto* NewHeap = new FTactixAgentScheduleEntry[NewCapacity];
		if (Count > 0 && Heap != nullptr)
		{
			for (std::size_t i = 0; i < Count; ++i) NewHeap[i] = Heap[i];
		}
		delete[] Heap;
		Heap = NewHeap;
		Cap  = NewCapacity;
	}

	std::size_t FTactixAgentScheduler::FindIndex(FTactixHandle<ITactixAgent> Agent) const
	{
		for (std::size_t i = 0; i < Count; ++i)
		{
			if (Heap[i].Agent == Agent) return i;
		}
		return Cap; // sentinel meaning "not found"
	}

	bool FTactixAgentScheduler::Contains(FTactixHandle<ITactixAgent> Agent) const
	{
		return FindIndex(Agent) != Cap;
	}

	bool FTactixAgentScheduler::Register(const FTactixAgentScheduleEntry& Entry)
	{
		if (!Entry.Agent.IsValid()) return false;

		const std::size_t Existing = FindIndex(Entry.Agent);
		if (Existing != Cap)
		{
			// Replace-in-place and re-sift both directions (either may be required).
			Heap[Existing] = Entry;
			SiftUp(Existing);
			SiftDown(Existing);
			return true;
		}

		if (Count == Cap) Grow(Cap * 2);
		Heap[Count] = Entry;
		SiftUp(Count);
		++Count;
		return true;
	}

	bool FTactixAgentScheduler::Unregister(FTactixHandle<ITactixAgent> Agent)
	{
		const std::size_t Idx = FindIndex(Agent);
		if (Idx == Cap) return false;
		--Count;
		if (Idx != Count)
		{
			Heap[Idx] = Heap[Count];
			SiftUp(Idx);
			SiftDown(Idx);
		}
		return true;
	}

	void FTactixAgentScheduler::SiftUp(std::size_t Index)
	{
		while (Index > 0)
		{
			const std::size_t Parent = (Index - 1) / 2;
			if (Less(Heap[Index], Heap[Parent]))
			{
				std::swap(Heap[Index], Heap[Parent]);
				Index = Parent;
			}
			else
			{
				break;
			}
		}
	}

	void FTactixAgentScheduler::SiftDown(std::size_t Index)
	{
		while (true)
		{
			const std::size_t Left  = Index * 2 + 1;
			const std::size_t Right = Index * 2 + 2;
			std::size_t       Smallest = Index;

			if (Left  < Count && Less(Heap[Left],  Heap[Smallest])) Smallest = Left;
			if (Right < Count && Less(Heap[Right], Heap[Smallest])) Smallest = Right;
			if (Smallest == Index) return;

			std::swap(Heap[Smallest], Heap[Index]);
			Index = Smallest;
		}
	}

	std::size_t FTactixAgentScheduler::Collect(double                       NowSeconds,
	                                           uint32_t                     FrameBudgetUs,
	                                           FTactixHandle<ITactixAgent>* OutAgents,
	                                           std::size_t                  OutCapacity)
	{
		if (OutAgents == nullptr || OutCapacity == 0) return 0;

		uint32_t    SpentUs = 0;
		std::size_t NumOut  = 0;

		while (Count > 0 && NumOut < OutCapacity)
		{
			FTactixAgentScheduleEntry& Top = Heap[0];
			if (Top.NextTickTime > NowSeconds) break;

			// Budget check: if adding this agent's expected cost would blow the
			// frame budget, stop — but always allow the very first agent to
			// run, otherwise an expensive agent could starve forever.
			const uint32_t ExpectedUs = Top.LastSpentUs == 0 ? Top.BudgetUs : Top.LastSpentUs;
			if (NumOut > 0 && SpentUs + ExpectedUs > FrameBudgetUs) break;

			OutAgents[NumOut++] = Top.Agent;
			SpentUs            += ExpectedUs;

			// Re-schedule at now + interval so we don't double-tick.
			Top.NextTickTime = NowSeconds + (double)Top.TickIntervalMs / 1000.0;
			SiftDown(0);
		}
		return NumOut;
	}

	void FTactixAgentScheduler::ReportTickCost(FTactixHandle<ITactixAgent> Agent, uint32_t SpentUs)
	{
		const std::size_t Idx = FindIndex(Agent);
		if (Idx == Cap) return;
		Heap[Idx].LastSpentUs = SpentUs;
	}
}
