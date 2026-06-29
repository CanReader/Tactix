// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixDebug.h
 * @brief Module object for TactixDebug, the editor/development tooling layer.
 *
 * Everything here is for looking at the AI, not running it: world-space agent
 * overlays, the influence-map heat renderer, a per-agent timing profiler, a
 * decision recorder for post-mortems, and a Gameplay Debugger category that ties
 * them together. None of it ships in a shipping build.
 */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * @brief Module implementation for TactixDebug.
 *
 * Startup registers the Gameplay Debugger category (when that subsystem is
 * present); shutdown unregisters it.
 */
class TACTIXDEBUG_API FTactixDebugModule : public IModuleInterface
{
public:
	/** @brief Registers debug integrations on module load. */
	virtual void StartupModule()  override;
	/** @brief Tears them down on module unload. */
	virtual void ShutdownModule() override;
};
