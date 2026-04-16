// Copyright Sleak Software. All Rights Reserved.

#include "Subsystem/UTactixWorldSubsystem.h"

void UTactixWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// CoverSystem and InfluenceMap are already live with the defaults declared
	// in the header. Runtime config could reconstruct the map here if needed.
}
