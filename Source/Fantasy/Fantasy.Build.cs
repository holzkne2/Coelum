// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Fantasy : ModuleRules
{
	public Fantasy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "UMG", "Slate", "SlateCore", "ProceduralMeshComponent" });
	}
}
