// Copyright Sleak Software. All Rights Reserved.

using UnrealBuildTool;

public class TactixSystems : ModuleRules
{
	public TactixSystems(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		IWYUSupport = IWYUSupport.Full;
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"TactixCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});
	}
}
