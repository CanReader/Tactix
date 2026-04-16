// Copyright Sleak Software. All Rights Reserved.
//
// Drop this component on any APawn that needs to participate in Tactix AI
// systems (utility scoring, cover claiming, squad membership, influence
// stamping). It is the ONLY bridge point between UE actor space and the
// engine-agnostic Tactix::ITactixAgent interface — everything inside
// TactixCore/TactixSystems sees this agent exclusively through its handle.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Agent/ITactixAgent.h"
#include "Agent/FTactixAgentContext.h"
#include "UTactixAgentComponent.generated.h"

UCLASS(ClassGroup = AI, meta = (BlueprintSpawnableComponent))
class TACTIXUE_API UTactixAgentComponent : public UActorComponent, public Tactix::ITactixAgent
{
	GENERATED_BODY()

public:
	UTactixAgentComponent();

	// ---- ITactixAgent -------------------------------------------------------

	Tactix::FTactixAgentContext          BuildContext() const override;
	Tactix::FTactixHandle<Tactix::ITactixAgent> GetHandle()    const override;

	// Set once by ATactixAIController::OnPossess; stable for the pawn's life.
	void SetHandle(Tactix::FTactixHandle<Tactix::ITactixAgent> InHandle);

	// ---- Vital state --------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float MaxAmmo = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float CurrentAmmo = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|Vitals")
	float CurrentStamina = 100.0f;

	// ---- Tactical state (written by BT tasks / Blueprint) ------------------

	UPROPERTY(BlueprintReadWrite, Category = "Tactix|State")
	bool bInCover = false;

	UPROPERTY(BlueprintReadWrite, Category = "Tactix|State")
	bool bReloading = false;

	UPROPERTY(BlueprintReadWrite, Category = "Tactix|State")
	FVector ThreatLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Tactix|State")
	bool bHasThreat = false;

	// Used to normalise raw threat distance to [0, 1] in consideration inputs.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactix|State", meta = (ClampMin = "1.0"))
	float MaxThreatDistance = 5000.0f;

private:
	Tactix::FTactixHandle<Tactix::ITactixAgent> Handle{};
};
