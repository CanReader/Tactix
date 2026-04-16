// Copyright Sleak Software. All Rights Reserved.

using UnrealBuildTool;

public class TactixCore : ModuleRules
{
	public TactixCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		IWYUSupport = IWYUSupport.Full;
		bUseUnity = false;

		// Public headers must compile without engine types. We only depend on
		// "Core" so we can register the module with UE and declare a log category.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});
	}
}
