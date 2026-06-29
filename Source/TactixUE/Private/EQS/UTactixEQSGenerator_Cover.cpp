// Copyright Sleak Software. All Rights Reserved.

/**
 * @file UTactixEQSGenerator_Cover.cpp
 * @brief Implements the cover EQS generator: emit each registered cover point as
 *        a point item.
 */

#include "EQS/UTactixEQSGenerator_Cover.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "Subsystem/UTactixWorldSubsystem.h"

UTactixEQSGenerator_Cover::UTactixEQSGenerator_Cover()
{
	ItemType = UEnvQueryItemType_Point::StaticClass();
}

void UTactixEQSGenerator_Cover::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();
	if (!QueryOwner) return;

	UWorld* World = QueryOwner->GetWorld();
	if (!World) return;

	UTactixWorldSubsystem* TactixSub = World->GetSubsystem<UTactixWorldSubsystem>();
	if (!TactixSub) return;

	Tactix::FTactixCoverSystem<512>& CoverSys = TactixSub->GetCoverSystem();

	TArray<FNavLocation> CoverLocations;
	CoverSys.ForEach([&](Tactix::FTactixCoverSystem<512>::FHandle /*H*/,
	                      const Tactix::FTactixCoverPoint& P)
	{
		if (bExcludeClaimed && P.IsClaimed()) return;
		CoverLocations.Add(FNavLocation(FVector(P.Position.X, P.Position.Y, P.Position.Z)));
	});

	QueryInstance.ReserveItemData(CoverLocations.Num());
	for (const FNavLocation& Loc : CoverLocations)
	{
		QueryInstance.AddItemData<UEnvQueryItemType_Point>(Loc);
	}
}

FText UTactixEQSGenerator_Cover::GetDescriptionTitle() const
{
	return FText::FromString(TEXT("Tactix Cover Points"));
}

FText UTactixEQSGenerator_Cover::GetDescriptionDetails() const
{
	return FText::FromString(
	    bExcludeClaimed ? TEXT("Unclaimed cover points only") : TEXT("All registered cover points"));
}
