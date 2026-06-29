// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixWorldSubsystem.cpp
 * @brief Implements the world subsystem: the soft cover-reservation layer.
 *
 * The cover system and influence map need no setup beyond their member
 * initialisers. Reservations are kept in a small map keyed by cover actor and
 * checked against game time on lookup, so expired entries simply read as absent;
 * they're overwritten on the next reservation rather than swept eagerly.
 */

#include "Subsystem/UTactixWorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void UTactixWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// CoverSystem and InfluenceMap are already live with the defaults declared
	// in the header. Runtime config could reconstruct the map here if needed.
}

void UTactixWorldSubsystem::ReserveCover(const AActor* Reserver, const AActor* CoverActor, float Lifetime)
{
	if (!Reserver || !CoverActor) return;
	const UWorld* W = GetWorld();
	if (!W) return;

	FCoverReservation R;
	R.Reserver  = Reserver;
	R.ExpiresAt = W->GetTimeSeconds() + Lifetime;
	CoverReservations.Add(CoverActor, R);
}

bool UTactixWorldSubsystem::IsCoverReservedByOther(const AActor* Reserver, const AActor* CoverActor) const
{
	if (!CoverActor) return false;
	const UWorld* W = GetWorld();
	if (!W) return false;

	const FCoverReservation* Existing = CoverReservations.Find(CoverActor);
	if (!Existing) return false;
	if (Existing->ExpiresAt <= W->GetTimeSeconds()) return false;
	const AActor* HeldBy = Existing->Reserver.Get();
	return HeldBy != nullptr && HeldBy != Reserver;
}
