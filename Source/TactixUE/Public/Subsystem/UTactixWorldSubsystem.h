// Copyright Sleak Software. All Rights Reserved.
//
// World-level Tactix systems shared by all agents in the level. Access via
//   GetWorld()->GetSubsystem<UTactixWorldSubsystem>()
//
// Channels on the default influence map:
//   0 — Danger    (enemy presence, projectile splash)
//   1 — Control   (friendly suppression, owned zones)
//   2 — Resource  (pickups, objectives, resupply)
//   3 — Custom    (project-defined)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Cover/FTactixCoverSystem.h"
#include "Spatial/FTactixInfluenceMap.h"
#include "UTactixWorldSubsystem.generated.h"

UCLASS()
class TACTIXUE_API UTactixWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	Tactix::FTactixCoverSystem<512>& GetCoverSystem()  { return CoverSystem; }
	Tactix::FTactixInfluenceMap<4>&  GetInfluenceMap() { return InfluenceMap; }

	// Grid footprint: InfluenceGridCells x InfluenceGridCells cells,
	// each InfluenceCellSize UU wide. Centred on world origin by default.
	// Changing these after Initialize has no effect on the live map.
	UPROPERTY(EditAnywhere, Category = "Tactix|InfluenceMap", meta = (ClampMin = "1"))
	int32 InfluenceGridCells = 64;

	UPROPERTY(EditAnywhere, Category = "Tactix|InfluenceMap", meta = (ClampMin = "1.0"))
	float InfluenceCellSize = 200.0f;

private:
	Tactix::FTactixCoverSystem<512> CoverSystem;
	// 64x64 cells, 200 UU each = 12,800 x 12,800 UU footprint.
	Tactix::FTactixInfluenceMap<4>  InfluenceMap{64, 64, 200.0f};
};
