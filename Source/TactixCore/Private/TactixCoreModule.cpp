// Copyright Sleak Software. All Rights Reserved.

#include "TactixCore.h"

DEFINE_LOG_CATEGORY(LogTactix);

#define LOCTEXT_NAMESPACE "FTactixCoreModule"

void FTactixCoreModule::StartupModule()
{
	UE_LOG(LogTactix, Log, TEXT("TactixCore v%d.%d.%d starting up"),
		Tactix::kVersionMajor, Tactix::kVersionMinor, Tactix::kVersionPatch);
}

void FTactixCoreModule::ShutdownModule()
{
	UE_LOG(LogTactix, Log, TEXT("TactixCore shutting down"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTactixCoreModule, TactixCore)
