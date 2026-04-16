// Copyright Sleak Software. All Rights Reserved.
//
// TactixUE module header. Unreal-facing integration lives here: AIController
// subclasses, Behavior Tree tasks/decorators/services, EQS generators,
// UDataAsset-backed tuning, and the UTactixAgentComponent bridge. Phase 3
// content.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class TACTIXUE_API FTactixUEModule : public IModuleInterface
{
public:
	virtual void StartupModule()  override;
	virtual void ShutdownModule() override;
};
