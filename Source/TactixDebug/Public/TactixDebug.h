// Copyright Sleak Software. All Rights Reserved.
//
// TactixDebug module header (Editor-only). Visual debuggers, agent profilers,
// decision recorders, and influence map renderers will live here. Phase 3.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class TACTIXDEBUG_API FTactixDebugModule : public IModuleInterface
{
public:
	virtual void StartupModule()  override;
	virtual void ShutdownModule() override;
};
