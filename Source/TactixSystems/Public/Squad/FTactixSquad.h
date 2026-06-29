// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixSquad.h
 * @brief Fixed-capacity squad: a roster of agent handles with roles, slots, a
 *        leader and a current tactic.
 *
 * The squad is pure bookkeeping. It tracks who is in the group, what role each
 * one plays, which formation slot they own, who leads, and what the group is
 * currently trying to do. It deliberately does no movement or decision-making
 * itself; the formation system reads slot indices from here, and higher-level AI
 * reads/sets the tactic.
 */

#pragma once

#include "TactixApi.h"
#include "Foundation/TactixHandle.h"
#include "Agent/ITactixAgent.h"

#include <cstddef>
#include <cstdint>

namespace Tactix
{
	/** @brief The group-level intent a squad is currently pursuing. */
	enum class ETactixSquadTactic : uint8_t
	{
		Idle,      ///< No active tactic.
		Advance,   ///< Move toward the objective/threat.
		Hold,      ///< Stay put and defend.
		Retreat,   ///< Fall back.
		Flank,     ///< Maneuver to attack from the side.
		Suppress,  ///< Pin the enemy with fire.
	};

	/** @brief One squad slot: which agent, its role, and its formation slot index. */
	struct FTactixSquadMember
	{
		FTactixHandle<ITactixAgent> Agent{};                       ///< The member.
		ETactixSquadRole            Role{ETactixSquadRole::None};  ///< Its tactical role.
		uint32_t                    SlotIndex{0};                  ///< Index into the active formation.
	};

	/**
	 * @brief A squad of up to @p Capacity agents.
	 * @tparam Capacity Maximum member count, fixed at compile time.
	 */
	template <std::size_t Capacity>
	class FTactixSquad
	{
		static_assert(Capacity > 0, "FTactixSquad must have a non-zero capacity.");

	public:
		/** @brief Shorthand for the agent handle type. */
		using FAgentHandle = FTactixHandle<ITactixAgent>;

		/**
		 * @brief Adds an agent to the squad.
		 * @param Agent Agent to add.
		 * @param Role  Initial role.
		 * @return False if the handle is invalid, the agent is already a member, or
		 *         the squad is full. New members get a slot index equal to their
		 *         insertion order.
		 */
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

		/**
		 * @brief Removes an agent from the squad.
		 * @param Agent Agent to remove.
		 * @return True if it was a member. Removal swaps in the last member, so the
		 *         roster order is not preserved (see @ref CompactSlots). If the
		 *         removed agent was the leader, the squad is left leaderless.
		 */
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

		/**
		 * @brief Looks up a member record.
		 * @param Agent Agent to find.
		 * @return Pointer to its record, or @c nullptr if not a member. Invalidated
		 *         by any @ref Add or @ref Remove.
		 */
		FTactixSquadMember* Find(FAgentHandle Agent)
		{
			for (uint32_t i = 0; i < MemberCount; ++i)
			{
				if (Members[i].Agent == Agent) return &Members[i];
			}
			return nullptr;
		}

		/** @brief Const overload of @ref Find. */
		const FTactixSquadMember* Find(FAgentHandle Agent) const
		{
			for (uint32_t i = 0; i < MemberCount; ++i)
			{
				if (Members[i].Agent == Agent) return &Members[i];
			}
			return nullptr;
		}

		/**
		 * @brief Changes a member's role.
		 * @return False if @p Agent isn't a member.
		 */
		bool AssignRole(FAgentHandle Agent, ETactixSquadRole NewRole)
		{
			FTactixSquadMember* M = Find(Agent);
			if (M == nullptr) return false;
			M->Role = NewRole;
			return true;
		}

		/**
		 * @brief Changes a member's formation slot index.
		 * @return False if @p Agent isn't a member.
		 */
		bool AssignSlot(FAgentHandle Agent, uint32_t SlotIndex)
		{
			FTactixSquadMember* M = Find(Agent);
			if (M == nullptr) return false;
			M->SlotIndex = SlotIndex;
			return true;
		}

		/**
		 * @brief Renumbers slots to 0..N-1 in current roster order.
		 *
		 * Useful after @ref Remove leaves holes in the slot numbering and you want a
		 * dense set of indices for the formation again.
		 */
		void CompactSlots()
		{
			for (uint32_t i = 0; i < MemberCount; ++i) Members[i].SlotIndex = i;
		}

		/**
		 * @brief Sets the squad leader.
		 * @param NewLeader Agent to lead, or an invalid handle to clear the leader.
		 *        A non-member handle is ignored (the leader is left unchanged).
		 */
		void SetLeader(FAgentHandle NewLeader)
		{
			if (!NewLeader.IsValid())        { Leader = FAgentHandle::Invalid(); return; }
			if (Find(NewLeader) == nullptr)  return;   // not a member
			Leader = NewLeader;
		}

		/** @brief Current leader, or an invalid handle if none. */
		FAgentHandle       GetLeader() const { return Leader; }
		/** @brief Sets the squad's current tactic. */
		void               SetTactic(ETactixSquadTactic T) { Tactic = T; }
		/** @brief Current tactic. */
		ETactixSquadTactic GetTactic() const { return Tactic; }

		/** @brief Member count. */
		uint32_t                    Num()                   const { return MemberCount; }
		/** @brief Pointer to the member array; valid for @ref Num entries. */
		const FTactixSquadMember*   GetMembers()             const { return Members; }
		/** @brief Const member access by roster index. No bounds check. */
		const FTactixSquadMember&   GetMember(uint32_t Idx)  const { return Members[Idx]; }
		/** @brief Mutable member access by roster index. No bounds check. */
		FTactixSquadMember&         GetMember(uint32_t Idx)        { return Members[Idx]; }

	private:
		FTactixSquadMember  Members[Capacity]{};                ///< Roster, packed in [0, MemberCount).
		uint32_t            MemberCount{0};                     ///< Live member count.
		FAgentHandle        Leader{};                           ///< Current leader, if any.
		ETactixSquadTactic  Tactic{ETactixSquadTactic::Idle};   ///< Current group tactic.
	};
}
