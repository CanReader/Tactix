// Copyright Sleak Software. All Rights Reserved.

/**
 * @file FTactixGameplayDebuggerCategory.h
 * @brief Tactix category for Unreal's built-in Gameplay Debugger.
 *
 * Surfaces agent vitals, cover state, the active plan and recent decisions in the
 * standard Gameplay Debugger overlay (apostrophe key, or `showdebug tactix`), so
 * none of it needs Blueprint wiring to inspect. The category is registered in
 * @ref FTactixDebugModule::StartupModule and removed on shutdown.
 *
 * @note Guarded by @c WITH_GAMEPLAY_DEBUGGER; compiles to nothing where that
 *       subsystem isn't available.
 */

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

/** @brief Gameplay Debugger category that reports Tactix agent state. */
class TACTIXDEBUG_API FTactixGameplayDebuggerCategory : public FGameplayDebuggerCategory
{
public:
	FTactixGameplayDebuggerCategory();

	/**
	 * @brief Gathers debug text for the inspected actor.
	 * @param OwnerPC    The debugging player controller.
	 * @param DebugActor The actor currently selected in the debugger.
	 */
	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	/**
	 * @brief Renders the collected data to the debugger canvas.
	 * @param OwnerPC       The debugging player controller.
	 * @param CanvasContext Canvas to draw into.
	 */
	virtual void DrawData  (APlayerController* OwnerPC,
	                        FGameplayDebuggerCanvasContext& CanvasContext) override;

	/** @brief Factory used to register the category with the debugger. */
	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	// Data goes out through AddTextLine, which the base class replicates for us,
	// so there's no custom FDataPack serialization to maintain.
};
#endif // WITH_GAMEPLAY_DEBUGGER
