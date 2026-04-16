// Copyright Sleak Software. All Rights Reserved.

using UnrealBuildTool;

public class TactixDebug : ModuleRules
{
	public TactixDebug(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;
		IWYUSupport = IWYUSupport.Full;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"TactixCore",
			"TactixSystems",
			"TactixUE",
			"GameplayDebugger",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"InputCore",
			"UnrealEd",
			"EditorStyle",
			"EditorFramework",
			"ToolMenus",
		});
	}
}
