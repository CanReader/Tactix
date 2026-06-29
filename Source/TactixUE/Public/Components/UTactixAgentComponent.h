// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixAgentComponent.h
 * @brief The component that makes a pawn a Tactix agent.
 *
 * Add this to any @c APawn that should take part in the Tactix AI systems:
 * utility scoring, cover claiming, squad membership, influence stamping. It is
 * the single bridge between Unreal actor space and the engine-agnostic
 * @ref Tactix::ITactixAgent interface. Once registered, every system in
 * TactixCore and TactixSystems refers to this agent purely by its handle, never
 * by an actor pointer, so the AI never reaches back into engine types.
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Agent/ITactixAgent.h"
#include "Agent/FTactixAgentContext.h"
#include "UTactixAgentComponent.generated.h"

/**
 * @brief Adapts a UE pawn to @ref Tactix::ITactixAgent and holds its AI state.
 *
 * The vital and tactical @c UPROPERTY fields are the source of truth that
 * @ref BuildContext snapshots each decision. Blueprint and BT tasks write to
 * them (taking damage, spending ammo, entering cover); the AI only ever reads the
 * snapshot.
 */
UCLASS(ClassGroup = AI, meta = (BlueprintSpawnableComponent))
class TACTIXUE_API UTactixAgentComponent : public UActorComponent, public Tactix::ITactixAgent
{
	GENERATED_BODY()

public:
	UTactixAgentComponent();

	/**
	 * @brief Builds the agent snapshot from the current component state.
	 *
	 * Converts the UE-side fields into a @ref Tactix::FTactixAgentContext "FTactixAgentContext": vitals
	 * become ratios, the owner's transform fills position/velocity/forward, and
	 * threat distance is derived from @c ThreatLocation.
	 * @return The populated context for this frame.
	 */
	Tactix::FTactixAgentContext          BuildContext() const override;

	/** @brief This agent's stable handle. @see SetHandle */
	Tactix::FTactixHandle<Tactix::ITactixAgent> GetHandle()    const override;

	/**
	 * @brief Assigns the agent's handle.
	 * @param InHandle Handle minted by the owning controller.
	 * @note Called once from @ref ATactixAIController::OnPossess. The handle then
	 *       stays fixed for the pawn's lifetime, which is what other systems rely
	 *       on to reference it across frames.
	 */
	void SetHandle(Tactix::FTactixHandle<Tactix::ITactixAgent> InHandle);

	/** @brief Maximum health, denominator for the health ratio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float MaxHealth = 100.0f;

	/** @brief Current health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float CurrentHealth = 100.0f;

	/** @brief Maximum ammo, denominator for the ammo ratio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float MaxAmmo = 100.0f;

	/** @brief Current ammo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float CurrentAmmo = 100.0f;

	/** @brief Maximum stamina, denominator for the stamina ratio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float MaxStamina = 100.0f;

	/** @brief Current stamina. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float CurrentStamina = 100.0f;

	/** @brief Whether the agent is currently in cover. Written by BT/Blueprint. */
	UPROPERTY(BlueprintReadWrite, Category = "Tactix|State")
	bool bInCover = false;

	/** @brief Whether a reload is in progress. */
	UPROPERTY(BlueprintReadWrite, Category = "Tactix|State")
	bool bReloading = false;

	/** @brief World position of the current threat; only meaningful when @c bHasThreat. */
	UPROPERTY(BlueprintReadWrite, Category = "Tactix|State")
	FVector ThreatLocation = FVector::ZeroVector;

	/** @brief Whether the agent currently has a threat. */
	UPROPERTY(BlueprintReadWrite, Category = "Tactix|State")
	bool bHasThreat = false;

	/** @brief Distance (UU) used to normalise threat distance into [0, 1] for considerations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|State", meta = (ClampMin = "1.0"))
	float MaxThreatDistance = 5000.0f;

private:
	/** @brief Stable identity assigned by the controller; invalid until @ref SetHandle. */
	Tactix::FTactixHandle<Tactix::ITactixAgent> Handle{};
};
