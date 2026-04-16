// Copyright Sleak Software. All Rights Reserved.
//
// TactixSystems module header. L3 behaviour systems (Utility AI, GOAP, HTN,
// Cover, InfluenceMap, Squad, Formation) live under this module. The public
// Phase 1 surface is intentionally empty — this file is a placeholder so the
// plugin's four-module shape is in place from day one and Phase 2 can drop in
// implementations without touching plugin wiring.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class TACTIXSYSTEMS_API FTactixSystemsModule : public IModuleInterface
{
public:
	virtual void StartupModule()  override;
	virtual void ShutdownModule() override;
};
