// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixAgentComponent.cpp
 * @brief Implements the pawn-to-agent adapter: snapshotting UE state into a
 *        @ref Tactix::FTactixAgentContext.
 */

#include "Components/UTactixAgentComponent.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

using namespace Tactix;

UTactixAgentComponent::UTactixAgentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FTactixAgentContext UTactixAgentComponent::BuildContext() const
{
	FTactixAgentContext Ctx{};
	Ctx.AgentHandle = Handle;

	const AActor* Owner = GetOwner();
	if (Owner)
	{
		const FVector Pos = Owner->GetActorLocation();
		Ctx.Position = { (float)Pos.X, (float)Pos.Y, (float)Pos.Z };

		const FVector Fwd = Owner->GetActorForwardVector();
		Ctx.Forward = { (float)Fwd.X, (float)Fwd.Y, (float)Fwd.Z };

		// Try to get velocity from the root primitive's physics state.
		if (UPrimitiveComponent* Root =
		        Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
		{
			const FVector Vel = Root->GetPhysicsLinearVelocity();
			Ctx.Velocity = { (float)Vel.X, (float)Vel.Y, (float)Vel.Z };
		}
	}

	// Normalise vitals; guards against zero max values.
	Ctx.HealthRatio  = (MaxHealth  > 0.0f) ? FMath::Clamp(CurrentHealth  / MaxHealth,  0.0f, 1.0f) : 0.0f;
	Ctx.AmmoRatio    = (MaxAmmo    > 0.0f) ? FMath::Clamp(CurrentAmmo    / MaxAmmo,    0.0f, 1.0f) : 0.0f;
	Ctx.StaminaRatio = (MaxStamina > 0.0f) ? FMath::Clamp(CurrentStamina / MaxStamina, 0.0f, 1.0f) : 0.0f;

	Ctx.bInCover   = bInCover;
	Ctx.bReloading = bReloading;

	if (bHasThreat && Owner)
	{
		Ctx.ThreatLocation = { (float)ThreatLocation.X, (float)ThreatLocation.Y, (float)ThreatLocation.Z };
		Ctx.ThreatDistance = (float)(Owner->GetActorLocation() - ThreatLocation).Size();
	}

	if (const UWorld* W = GetWorld())
	{
		Ctx.TimeSeconds = W->GetTimeSeconds();
	}

	return Ctx;
}

FTactixHandle<ITactixAgent> UTactixAgentComponent::GetHandle() const
{
	return Handle;
}

void UTactixAgentComponent::SetHandle(FTactixHandle<ITactixAgent> InHandle)
{
	Handle = InHandle;
}
