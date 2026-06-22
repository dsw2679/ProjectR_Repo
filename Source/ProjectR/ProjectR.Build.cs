// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectR : ModuleRules
{
	public ProjectR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"AIModule",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"DeveloperSettings",
			"NavigationSystem",
			"MotionWarping",
			"UMG",
			"NetCore",
			"StructUtils",
			"Niagara",
			"PhysicsCore",
			"Slate",
			"SlateCore",
			"OnlineSubsystem",
			"LevelSequence",
			"MovieScene",
			"MovieSceneTracks",
			"CommonUI",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AnimGraphRuntime", "OnlineSubsystemUtils" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
