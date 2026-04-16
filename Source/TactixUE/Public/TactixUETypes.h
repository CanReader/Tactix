// Copyright Sleak Software. All Rights Reserved.
//
// UE-facing enum and struct mirrors for Tactix types that cannot carry
// UPROPERTY/UENUM in TactixCore or TactixSystems (no UE headers allowed
// in those modules). Converting between the BP and Tactix variants happens
// inside TactixUE — the engine-agnostic layers never see this file.

#pragma once

#include "CoreMinimal.h"
#include "TactixUETypes.generated.h"

// ---- Response curve selection ----------------------------------------------

UENUM(BlueprintType)
enum class ETactixCurveTypeBP : uint8
{
	Linear      UMETA(DisplayName = "Linear"),
	Quadratic   UMETA(DisplayName = "Quadratic"),
	Logistic    UMETA(DisplayName = "Logistic"),
	Exponential UMETA(DisplayName = "Exponential"),
};

// ---- Utility consideration input signals -----------------------------------
// Each value maps to one field on FTactixAgentContext. Add entries as the
// context grows; keep the mapping in UTactixUtilityAsset::GetRawInput().

UENUM(BlueprintType)
enum class ETactixInputType : uint8
{
	HealthRatio    UMETA(DisplayName = "Health Ratio"),
	AmmoRatio      UMETA(DisplayName = "Ammo Ratio"),
	StaminaRatio   UMETA(DisplayName = "Stamina Ratio"),
	ThreatDistance UMETA(DisplayName = "Threat Distance (normalized)"),
	Speed          UMETA(DisplayName = "Speed (normalized, 600 uu/s cap)"),
	InCover        UMETA(DisplayName = "In Cover (0 or 1)"),
};

// ---- Formation / squad mirrors ---------------------------------------------

UENUM(BlueprintType)
enum class ETactixFormationKindBP : uint8
{
	Line    UMETA(DisplayName = "Line"),
	Column  UMETA(DisplayName = "Column"),
	Wedge   UMETA(DisplayName = "Wedge"),
	Box     UMETA(DisplayName = "Box"),
	Diamond UMETA(DisplayName = "Diamond"),
};

UENUM(BlueprintType)
enum class ETactixSquadTacticBP : uint8
{
	Idle     UMETA(DisplayName = "Idle"),
	Advance  UMETA(DisplayName = "Advance"),
	Hold     UMETA(DisplayName = "Hold"),
	Retreat  UMETA(DisplayName = "Retreat"),
	Flank    UMETA(DisplayName = "Flank"),
	Suppress UMETA(DisplayName = "Suppress"),
};

UENUM(BlueprintType)
enum class ETactixSquadRoleBP : uint8
{
	None       UMETA(DisplayName = "None"),
	Leader     UMETA(DisplayName = "Leader"),
	Flanker    UMETA(DisplayName = "Flanker"),
	Suppressor UMETA(DisplayName = "Suppressor"),
	Medic      UMETA(DisplayName = "Medic"),
	Scout      UMETA(DisplayName = "Scout"),
	Assault    UMETA(DisplayName = "Assault"),
};

// ---- Consideration config (stored in UTactixUtilityAsset per action) ------

USTRUCT(BlueprintType)
struct TACTIXUE_API FTactixConsiderationConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	ETactixInputType InputType = ETactixInputType::HealthRatio;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	ETactixCurveTypeBP CurveType = ETactixCurveTypeBP::Linear;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	float Slope = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	float Exponent = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	float Shift = 0.0f;

	// Only used when InputType == ThreatDistance. Raw UU distance mapped to [0,1].
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility",
	          meta = (EditCondition = "InputType==ETactixInputType::ThreatDistance", ClampMin = "1.0"))
	float ThreatDistanceCap = 2000.0f;
};

// ---- Action config (one entry per scored action in UTactixUtilityAsset) ---

USTRUCT(BlueprintType)
struct TACTIXUE_API FTactixActionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	FName ActionName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	TArray<FTactixConsiderationConfig> Considerations;
};

// ---- Role slot config (one entry per role in UTactixSquadConfigAsset) -----

USTRUCT(BlueprintType)
struct TACTIXUE_API FTactixRoleConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	ETactixSquadRoleBP Role = ETactixSquadRoleBP::None;

	// How many agents with this role the squad allows at once.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad", meta = (ClampMin = "0"))
	int32 MaxCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	FText DisplayName;
};
