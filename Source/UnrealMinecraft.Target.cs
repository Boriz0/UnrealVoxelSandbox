// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealMinecraftTarget : TargetRules
{
	public UnrealMinecraftTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("UnrealMinecraft");
		PreBuildSteps.Add("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"$(ProjectDir)/Build/GenerateBuildMetadata.ps1\" -ProjectDirectory \"$(ProjectDir)\" -TargetName \"$(TargetName)\"");
	}
}
