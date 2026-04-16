// Copyright Sleak Software. All Rights Reserved.
//
// FTactixGameplayDebuggerCategory — integrates Tactix debug data into UE's
// built-in Gameplay Debugger (toggle with the apostrophe key, or via
// `showdebug tactix` in console). Displays agent vitals, cover state, active
// plan, and recent decisions without any extra Blueprint plumbing.
//
// Registered by FTactixDebugModule::StartupModule; unregistered on shutdown.

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

class TACTIXDEBUG_API FTactixGameplayDebuggerCategory : public FGameplayDebuggerCategory
{
public:
	FTactixGameplayDebuggerCategory();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData  (APlayerController* OwnerPC,
	                        FGameplayDebuggerCanvasContext& CanvasContext) override;

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	// All data is sent via AddTextLine (auto-replicated by the base class).
	// No custom FDataPack serialization needed.
};
#endif // WITH_GAMEPLAY_DEBUGGER
