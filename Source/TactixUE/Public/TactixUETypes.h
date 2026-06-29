// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixUETypes.h
 * @brief Blueprint-exposed mirrors of Tactix enums and config structs.
 *
 * The engine-agnostic modules can't carry @c UENUM / @c UPROPERTY metadata,
 * because they're forbidden from including UE headers. So the designer-facing
 * variants live here, in TactixUE, with a "BP" suffix where they shadow a core
 * enum. Conversion between these and the real Tactix types happens inside this
 * module (for example in @ref UTactixUtilityAsset); the core never sees this file.
 */

#pragma once

#include "CoreMinimal.h"
#include "TactixUETypes.generated.h"

/** @brief Blueprint mirror of @ref Tactix::ETactixCurveType (minus Custom, which needs a function pointer). */
UENUM(BlueprintType)
enum class ETactixCurveTypeBP : uint8
{
	Linear      UMETA(DisplayName = "Linear"),
	Quadratic   UMETA(DisplayName = "Quadratic"),
	Logistic    UMETA(DisplayName = "Logistic"),
	Exponential UMETA(DisplayName = "Exponential"),
};

/**
 * @brief Which agent-context field a consideration reads as its raw input.
 *
 * Each value names one field of @ref Tactix::FTactixAgentContext. When the
 * context grows, add an entry here and extend the mapping in
 * @c UTactixUtilityAsset::GetRawInput so the two stay in sync.
 */
UENUM(BlueprintType)
enum class ETactixInputType : uint8
{
	HealthRatio    UMETA(DisplayName = "Health Ratio"),                   ///< Context HealthRatio, already [0, 1].
	AmmoRatio      UMETA(DisplayName = "Ammo Ratio"),                     ///< Context AmmoRatio, already [0, 1].
	StaminaRatio   UMETA(DisplayName = "Stamina Ratio"),                  ///< Context StaminaRatio, already [0, 1].
	ThreatDistance UMETA(DisplayName = "Threat Distance (normalized)"),   ///< Threat distance normalised by @ref FTactixConsiderationConfig::ThreatDistanceCap.
	Speed          UMETA(DisplayName = "Speed (normalized, 600 uu/s cap)"),///< Velocity magnitude normalised against a 600 uu/s cap.
	InCover        UMETA(DisplayName = "In Cover (0 or 1)"),              ///< 1 when in cover, else 0.
};

/** @brief Blueprint mirror of @ref Tactix::ETactixFormationKind (named shapes only). */
UENUM(BlueprintType)
enum class ETactixFormationKindBP : uint8
{
	Line    UMETA(DisplayName = "Line"),
	Column  UMETA(DisplayName = "Column"),
	Wedge   UMETA(DisplayName = "Wedge"),
	Box     UMETA(DisplayName = "Box"),
	Diamond UMETA(DisplayName = "Diamond"),
};

/** @brief Blueprint mirror of @ref Tactix::ETactixSquadTactic. */
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

/** @brief Blueprint mirror of @ref Tactix::ETactixSquadRole (named roles only). */
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

/**
 * @brief One consideration's designer-tunable settings.
 *
 * Maps to a @ref Tactix::FTactixConsideration "FTactixConsideration": @c InputType picks the raw signal,
 * the remaining fields describe the response curve applied to it.
 */
USTRUCT(BlueprintType)
struct TACTIXUE_API FTactixConsiderationConfig
{
	GENERATED_BODY()

	/** @brief Which context field to read. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	ETactixInputType InputType = ETactixInputType::HealthRatio;

	/** @brief Response curve shape. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	ETactixCurveTypeBP CurveType = ETactixCurveTypeBP::Linear;

	/** @brief Curve gain (steepness for Logistic). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	float Slope = 1.0f;

	/** @brief Curve exponent (Quadratic/Exponential only). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	float Exponent = 2.0f;

	/** @brief Curve input offset (midpoint for Logistic). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	float Shift = 0.0f;

	/**
	 * @brief Distance (UU) that maps to 1.0 for the ThreatDistance input.
	 *
	 * Only meaningful when @c InputType is ThreatDistance; the editor hides it
	 * otherwise. Raw distance is divided by this before the curve runs.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility",
	          meta = (EditCondition = "InputType==ETactixInputType::ThreatDistance", ClampMin = "1.0"))
	float ThreatDistanceCap = 2000.0f;
};

/** @brief One scored action: a name plus the considerations that score it. */
USTRUCT(BlueprintType)
struct TACTIXUE_API FTactixActionConfig
{
	GENERATED_BODY()

	/** @brief Identifier written to the blackboard when this action wins. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	FName ActionName;

	/** @brief Considerations multiplied together (with Lewis compensation) for the score. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Utility")
	TArray<FTactixConsiderationConfig> Considerations;
};

/** @brief One squad role slot definition: which role, how many, and a label. */
USTRUCT(BlueprintType)
struct TACTIXUE_API FTactixRoleConfig
{
	GENERATED_BODY()

	/** @brief The role this entry configures. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	ETactixSquadRoleBP Role = ETactixSquadRoleBP::None;

	/** @brief How many agents may hold this role in the squad at once. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad", meta = (ClampMin = "0"))
	int32 MaxCount = 1;

	/** @brief Friendly name for UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tactix|Squad")
	FText DisplayName;
};
