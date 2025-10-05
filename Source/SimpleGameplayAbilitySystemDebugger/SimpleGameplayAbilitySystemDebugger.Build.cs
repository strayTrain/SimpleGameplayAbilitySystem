using UnrealBuildTool;

public class SimpleGameplayAbilitySystemDebugger : ModuleRules
{
	public SimpleGameplayAbilitySystemDebugger(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { });
		PrivateIncludePaths.AddRange(new string[] { });

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
			}
		);

		// For UE versions before 5.5, add the StructUtils dependency as it was an external plugin.
		if (Target.Version.MajorVersion < 5 || (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion < 5))
		{
			PublicDependencyModuleNames.Add("StructUtils");
		}

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"InputCore",
				"SimpleGameplayAbilitySystem",
			}
		);

		// Only include GameplayDebugger in Development and Debug builds
		if (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test)
		{
			PrivateDependencyModuleNames.Add("GameplayDebugger");
			PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
		}

		DynamicallyLoadedModuleNames.AddRange(new string[] { });
	}
}
