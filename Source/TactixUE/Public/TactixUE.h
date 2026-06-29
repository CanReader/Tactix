// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixUE.h
 * @brief Module object for TactixUE, the engine-integration layer.
 *
 * This is where the pure-C++ AI meets Unreal. The module's headers provide the
 * AIController base, Behavior Tree tasks/decorators/services, EQS generators and
 * tests, the designer-facing data assets, the world subsystem that owns the
 * shared cover and influence systems, and the @c UTactixAgentComponent that
 * adapts a pawn to @ref Tactix::ITactixAgent. This file itself only holds the
 * lifecycle object.
 */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** @brief Module implementation for TactixUE. Lifecycle logging only. */
class TACTIXUE_API FTactixUEModule : public IModuleInterface
{
public:
	/** @brief Called on module load. */
	virtual void StartupModule()  override;
	/** @brief Called on module unload. */
	virtual void ShutdownModule() override;
};
