// Copyright Sleak Software. All Rights Reserved.
//
// EQS generator: emits the world positions of all cover points registered in
// UTactixWorldSubsystem as EQS point items. Combine with
// UTactixEQSTest_Influence to score them by influence channel, or use the
// standard EQS distance/dot tests.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "UTactixEQSGenerator_Cover.generated.h"

UCLASS()
class TACTIXUE_API UTactixEQSGenerator_Cover : public UEnvQueryGenerator
{
	GENERATED_BODY()

public:
	UTactixEQSGenerator_Cover();

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle()  const override;
	virtual FText GetDescriptionDetails() const override;

	// If true, already-claimed points (by other agents) are not emitted.
	UPROPERTY(EditDefaultsOnly, Category = "Tactix|Cover")
	bool bExcludeClaimed = false;
};
