// Copyright Sleak Software. All Rights Reserved.

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Agent/ITactixAgent.h"

#include <cstddef>
#include <cstdint>

namespace Tactix
{
	enum class ETactixSquadTactic : uint8_t
	{
		Idle,
		Advance,
		Hold,
		Retreat,
		Flank,
		Suppress,
	};

	struct FTactixSquadMember
	{
		FTactixHandle<ITactixAgent> Agent{};
		ETactixSquadRole            Role{ETactixSquadRole::None};
		uint32_t                    SlotIndex{0};
	};

	template <std::size_t Capacity>
	class FTactixSquad
	{
		static_assert(Capacity > 0, "FTactixSquad must have a non-zero capacity.");

	public:
		using FAgentHandle = FTactixHandle<ITactixAgent>;

		bool Add(FAgentHandle Agent, ETactixSquadRole Role)
		{
			if (!Agent.IsValid())          return false;
			if (Find(Agent) != nullptr)    return false;   // already a member
			if (MemberCount >= Capacity)   return false;

			FTactixSquadMember& M = Members[MemberCount];
			M.Agent     = Agent;
			M.Role      = Role;
			M.SlotIndex = MemberCount;     // default slot = insertion order
			++MemberCount;
			return true;
		}

		bool Remove(FAgentHandle Agent)
		{
			for (uint32_t i = 0; i < MemberCount; ++i)
			{
				if (Members[i].Agent == Agent)
				{
					Members[i] = Members[MemberCount - 1u];
					--MemberCount;
					if (Leader == Agent) Leader = FAgentHandle::Invalid();
					return true;
				}
			}
			return false;
		}

		FTactixSquadMember* Find(FAgentHandle Agent)
		{
			for (uint32_t i = 0; i < MemberCount; ++i)
			{
				if (Members[i].Agent == Agent) return &Members[i];
			}
			return nullptr;
		}

		const FTactixSquadMember* Find(FAgentHandle Agent) const
		{
			for (uint32_t i = 0; i < MemberCount; ++i)
			{
				if (Members[i].Agent == Agent) return &Members[i];
			}
			return nullptr;
		}

		bool AssignRole(FAgentHandle Agent, ETactixSquadRole NewRole)
		{
			FTactixSquadMember* M = Find(Agent);
			if (M == nullptr) return false;
			M->Role = NewRole;
			return true;
		}

		bool AssignSlot(FAgentHandle Agent, uint32_t SlotIndex)
		{
			FTactixSquadMember* M = Find(Agent);
			if (M == nullptr) return false;
			M->SlotIndex = SlotIndex;
			return true;
		}

		// Pack SlotIndex to 0..N-1 in member order. Useful after several
		// Remove() calls leave gaps in the slot numbering.
		void CompactSlots()
		{
			for (uint32_t i = 0; i < MemberCount; ++i) Members[i].SlotIndex = i;
		}

		void SetLeader(FAgentHandle NewLeader)
		{
			if (!NewLeader.IsValid())        { Leader = FAgentHandle::Invalid(); return; }
			if (Find(NewLeader) == nullptr)  return;   // not a member
			Leader = NewLeader;
		}

		FAgentHandle       GetLeader() const { return Leader; }
		void               SetTactic(ETactixSquadTactic T) { Tactic = T; }
		ETactixSquadTactic GetTactic() const { return Tactic; }

		uint32_t                    Num()                   const { return MemberCount; }
		const FTactixSquadMember*   GetMembers()             const { return Members; }
		const FTactixSquadMember&   GetMember(uint32_t Idx)  const { return Members[Idx]; }
		FTactixSquadMember&         GetMember(uint32_t Idx)        { return Members[Idx]; }

	private:
		FTactixSquadMember  Members[Capacity]{};
		uint32_t            MemberCount{0};
		FAgentHandle        Leader{};
		ETactixSquadTactic  Tactic{ETactixSquadTactic::Idle};
	};
}
