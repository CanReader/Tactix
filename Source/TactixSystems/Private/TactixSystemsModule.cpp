// Copyright Sleak Software. All Rights Reserved.

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
