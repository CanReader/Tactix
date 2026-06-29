// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixSystems.h
 * @brief Unreal-facing module object for TactixSystems (L3 behaviour systems).
 *
 * This module is the home of the actual AI: Utility selection, the GOAP and HTN
 * planners, the cover registry, the influence map, and squad/formation logic.
 * All of it is engine-agnostic and depends only on TactixCore, so most of the
 * surface lives in headers under Utility/, GOAP/, HTN/, Squad/, Cover/ and
 * Spatial/. This file only carries the @c IModuleInterface object Unreal needs to
 * load and unload the module.
 */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * @brief Module implementation for TactixSystems.
 *
 * Lifecycle only logs today; the systems themselves are owned by whatever ticks
 * them (an AI controller, a world subsystem), not by the module object.
 */
class TACTIXSYSTEMS_API FTactixSystemsModule : public IModuleInterface
{
public:
	/** @brief Called on module load. */
	virtual void StartupModule()  override;
	/** @brief Called on module unload. */
	virtual void ShutdownModule() override;
};
