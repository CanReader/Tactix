// Copyright Sleak Software. All Rights Reserved.
//
// TactixCore module header. Include from UE-side code that needs:
//   • the `LogTactix` log category
//   • the module interface for runtime lifecycle hooks
//
// Pure-C++ consumers (unit tests, standalone tools) should include the
// individual Foundation/Agent/Perception/Blackboard headers directly and
// avoid this file — it pulls in CoreMinimal.h and UE's module system.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"
#include "TactixApi.h"

// ----------------------------------------------------------------------------
// Plugin-wide log category. DEFINE lives in Private/TactixCoreModule.cpp so
// all four Tactix modules share a single LogTactix channel.
// ----------------------------------------------------------------------------
TACTIXCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogTactix, Log, All);

class TACTIXCORE_API FTactixCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule()  override;
	virtual void ShutdownModule() override;
};
