// Copyright Sleak Software. All Rights Reserved.

#include "TactixDebug.h"
#include "TactixCore.h"
#include "FTactixGameplayDebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#endif

void FTactixDebugModule::StartupModule()
{
	UE_LOG(LogTactix, Log, TEXT("TactixDebug starting up"));

#if WITH_GAMEPLAY_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger& GDRef = IGameplayDebugger::Get();
		GDRef.RegisterCategory(
		    "Tactix",
		    IGameplayDebugger::FOnGetCategory::CreateStatic(
		        &FTactixGameplayDebuggerCategory::MakeInstance),
		    EGameplayDebuggerCategoryState::EnabledInGameAndSimulate,
		    5 /* slot index — shown as the 6th category tab */);
		GDRef.NotifyCategoriesChanged();
	}
#endif
}

void FTactixDebugModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger::Get().UnregisterCategory("Tactix");
	}
#endif

	UE_LOG(LogTactix, Log, TEXT("TactixDebug shutting down"));
}

IMPLEMENT_MODULE(FTactixDebugModule, TactixDebug)
