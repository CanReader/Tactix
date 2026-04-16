// Copyright Sleak Software. All Rights Reserved.
//
// Designer-facing data asset for squad configuration. Assign to a squad
// manager component or BTTask to control role counts, default formation,
// and starting tactic.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TactixUETypes.h"
#include "UTactixSquadConfigAsset.generated.h"

UCLASS(BlueprintType)
class TACTIXUE_API UTactixSquadConfigAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// Roles this squad type can have and how many of each are allowed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	TArray<FTactixRoleConfig> Roles;

	// Formation used when the squad is moving together.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	ETactixFormationKindBP DefaultFormation = ETactixFormationKindBP::Column;

	// Spacing in UU between formation slots.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad",
	          meta = (ClampMin = "10.0"))
	float FormationSpacing = 150.0f;

	// Tactic the squad enters when first assembled.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	ETactixSquadTacticBP DefaultTactic = ETactixSquadTacticBP::Idle;

	// Maximum agents this config supports.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad",
	          meta = (ClampMin = "1"))
	int32 MaxAgents = 8;
};
