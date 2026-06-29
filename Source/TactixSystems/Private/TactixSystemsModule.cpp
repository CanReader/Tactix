// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixSystemsModule.cpp
 * @brief Implements the TactixSystems module lifecycle (logging only).
 */

#include "TactixSystems.h"
#include "TactixCore.h"

void FTactixSystemsModule::StartupModule()
{
	UE_LOG(LogTactix, Log, TEXT("TactixSystems starting up"));
}

void FTactixSystemsModule::ShutdownModule()
{
	UE_LOG(LogTactix, Log, TEXT("TactixSystems shutting down"));
}

IMPLEMENT_MODULE(FTactixSystemsModule, TactixSystems)
