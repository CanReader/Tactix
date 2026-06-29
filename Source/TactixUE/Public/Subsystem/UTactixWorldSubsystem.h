// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixWorldSubsystem.h
 * @brief Per-world owner of the cover system and influence map shared by every
 *        agent in the level.
 *
 * One instance per world, reached with
 * `GetWorld()->GetSubsystem<UTactixWorldSubsystem>()`. It holds the two
 * level-global Tactix systems so agents don't each carry their own copy. The
 * default influence map uses four channels:
 *
 * | Channel | Meaning  | Typical sources                       |
 * |---------|----------|---------------------------------------|
 * | 0       | Danger   | enemy presence, projectile splash     |
 * | 1       | Control  | friendly suppression, owned zones     |
 * | 2       | Resource | pickups, objectives, resupply         |
 * | 3       | Custom   | project-defined                       |
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Cover/FTactixCoverSystem.h"
#include "Spatial/FTactixInfluenceMap.h"
#include "UTactixWorldSubsystem.generated.h"

/** @brief World subsystem holding the shared cover registry and influence map. */
UCLASS()
class TACTIXUE_API UTactixWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief Subsystem startup hook.
	 * @note The cover system and influence map are already live by this point,
	 *       constructed with the defaults baked into the member declarations below.
	 *       This is the place to rebuild the map from @ref InfluenceGridCells /
	 *       @ref InfluenceCellSize if runtime-configurable sizing is wired up.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** @brief The level's cover registry. */
	Tactix::FTactixCoverSystem<512>& GetCoverSystem()  { return CoverSystem; }
	/** @brief The level's influence map. */
	Tactix::FTactixInfluenceMap<4>&  GetInfluenceMap() { return InfluenceMap; }

	/**
	 * @brief Intended influence grid resolution (cells per side).
	 * @note Editor-facing knob for the map footprint. Not applied automatically
	 *       yet: the live map uses the default from the member initialiser below.
	 *       See @ref Initialize for where you'd hook up rebuilding.
	 */
	UPROPERTY(EditAnywhere, Category = "Tactix|InfluenceMap", meta = (ClampMin = "1"))
	int32 InfluenceGridCells = 64;

	/**
	 * @brief Intended world size (UU) of one influence cell.
	 * @note Pairs with @ref InfluenceGridCells to define the footprint. Like it,
	 *       this is not yet applied to the live map; see @ref Initialize.
	 */
	UPROPERTY(EditAnywhere, Category = "Tactix|InfluenceMap", meta = (ClampMin = "1.0"))
	float InfluenceCellSize = 200.0f;

	/**
	 * @brief Posts a short-lived "I'm heading for this cover" hint.
	 *
	 * This is a soft, non-authoritative layer on top of the cover system's hard
	 * claims. It nudges other agents to prefer different candidates so they don't
	 * all converge on the same cover before anyone has actually claimed it.
	 * Reservations expire, so agents re-post one each time they query.
	 *
	 * @param Reserver   Agent actor making the reservation.
	 * @param CoverActor The cover actor being targeted.
	 * @param Lifetime   Seconds the reservation stays live.
	 */
	void ReserveCover(const AActor* Reserver, const AActor* CoverActor, float Lifetime = 1.5f);

	/**
	 * @brief Whether some other agent currently has a live reservation on a cover.
	 * @param Reserver   The asking agent (its own reservation doesn't count).
	 * @param CoverActor The cover actor to check.
	 * @return True if a different, unexpired reserver holds it.
	 */
	bool IsCoverReservedByOther(const AActor* Reserver, const AActor* CoverActor) const;

private:
	Tactix::FTactixCoverSystem<512> CoverSystem; ///< Up to 512 cover points.
	/// 64x64 cells at 200 UU = a 12,800 x 12,800 UU footprint. This is the live size.
	Tactix::FTactixInfluenceMap<4>  InfluenceMap{64, 64, 200.0f};

	/** @brief A single soft cover reservation and when it lapses. */
	struct FCoverReservation
	{
		TWeakObjectPtr<const AActor> Reserver;       ///< Who reserved it.
		double                       ExpiresAt = 0.0; ///< Game time the reservation expires.
	};
	/** @brief Live reservations keyed by cover actor. */
	TMap<TWeakObjectPtr<const AActor>, FCoverReservation> CoverReservations;
};
