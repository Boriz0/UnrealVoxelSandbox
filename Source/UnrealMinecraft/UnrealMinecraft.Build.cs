// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealMinecraft : ModuleRules
{
	public UnrealMinecraft(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"ProceduralMeshComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "SlateCore", "PhysicsCore", "RenderCore" });

		PublicIncludePaths.AddRange(new string[] {
			"UnrealMinecraft",
			"UnrealMinecraft/Variant_Horror",
			"UnrealMinecraft/Variant_Horror/UI",
			"UnrealMinecraft/Variant_Shooter",
			"UnrealMinecraft/Variant_Shooter/AI",
			"UnrealMinecraft/Variant_Shooter/UI",
			"UnrealMinecraft/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
