// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixSquadConfigAsset.h
 * @brief Designer-facing asset describing a squad's roles, formation and tactic.
 *
 * Assign one to a squad manager component or a squad-setup task to drive how a
 * squad of this type is composed: which roles it allows and how many of each,
 * the formation it travels in, and the tactic it starts in.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TactixUETypes.h"
#include "UTactixSquadConfigAsset.generated.h"

/** @brief Data asset of squad composition and movement settings. */
UCLASS(BlueprintType)
class TACTIXUE_API UTactixSquadConfigAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** @brief Allowed roles and the per-role count caps. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	TArray<FTactixRoleConfig> Roles;

	/** @brief Formation used when the squad moves as a group. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	ETactixFormationKindBP DefaultFormation = ETactixFormationKindBP::Column;

	/** @brief Spacing in UU between formation slots. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad",
	          meta = (ClampMin = "10.0"))
	float FormationSpacing = 150.0f;

	/** @brief Tactic the squad starts in once assembled. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	ETactixSquadTacticBP DefaultTactic = ETactixSquadTacticBP::Idle;

	/** @brief Hard cap on members for this squad type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad",
	          meta = (ClampMin = "1"))
	int32 MaxAgents = 8;
};
