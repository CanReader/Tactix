// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixCore.h
 * @brief Unreal-facing entry point for the TactixCore module: the shared log
 *        category and the module lifecycle object.
 *
 * Include this from engine-side code that needs to log under @c LogTactix or
 * react to module startup/shutdown. It pulls in @c CoreMinimal.h and Unreal's
 * module manager, so it is the wrong header for the engine-agnostic layer.
 *
 * Pure-C++ consumers (the unit tests, any standalone tooling) should include the
 * specific Foundation/, Agent/, Perception/ or Blackboard/ header they need and
 * stay clear of this file, otherwise they drag in the whole engine.
 */

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "Modules/ModuleManager.h"
#include "TactixApi.h"

/**
 * @brief Single log channel shared by all four Tactix modules.
 *
 * Declared here, defined once in Private/TactixCoreModule.cpp. Keeping the
 * @c DEFINE_LOG_CATEGORY in TactixCore (the module everyone depends on) is what
 * lets TactixSystems, TactixUE and TactixDebug all log to the same channel
 * without each owning a duplicate category.
 */
TACTIXCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogTactix, Log, All);

/**
 * @brief Module implementation for TactixCore.
 *
 * Does no heavy lifting. Startup and shutdown only emit a version banner today,
 * but this is the hook to reach for if the foundation ever needs process-wide
 * setup (registering a console command, warming a pool) tied to module load.
 */
class TACTIXCORE_API FTactixCoreModule : public IModuleInterface
{
public:
	/** @brief Called when the module is loaded. Logs the Tactix version. */
	virtual void StartupModule()  override;
	/** @brief Called when the module is unloaded. */
	virtual void ShutdownModule() override;
};
