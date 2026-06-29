// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixCoreModule.cpp
 * @brief Implements the TactixCore module object and defines the shared log
 *        category for the whole plugin.
 */

#include "TactixCore.h"

/// Sole definition of @c LogTactix. The other Tactix modules only declare it.
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
