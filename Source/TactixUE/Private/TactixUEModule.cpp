// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixUEModule.cpp
 * @brief Implements the TactixUE module lifecycle (logging only).
 */

#include "TactixUE.h"
#include "TactixCore.h"

void FTactixUEModule::StartupModule()
{
	UE_LOG(LogTactix, Log, TEXT("TactixUE starting up"));
}

void FTactixUEModule::ShutdownModule()
{
	UE_LOG(LogTactix, Log, TEXT("TactixUE shutting down"));
}

IMPLEMENT_MODULE(FTactixUEModule, TactixUE)
