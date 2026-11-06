// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealMinecraftEditorTarget : TargetRules
{
	public UnrealMinecraftEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("UnrealMinecraft");
		PreBuildSteps.Add("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"$(ProjectDir)/Build/GenerateBuildMetadata.ps1\" -ProjectDirectory \"$(ProjectDir)\" -TargetName \"$(TargetName)\"");
	}
}
