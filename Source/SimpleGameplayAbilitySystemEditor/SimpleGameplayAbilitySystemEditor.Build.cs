using UnrealBuildTool;

public class SimpleGameplayAbilitySystemEditor : ModuleRules
{
	public SimpleGameplayAbilitySystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
		// Make sure our headers are visible to other modules
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
        
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add private include paths required here ...
			}
		);
        
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"SimpleGameplayAbilitySystem",
				"UnrealEd",
			}
		);
        
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"InputCore",
				"Slate",
				"SlateCore",
				"PropertyEditor", 
				"EditorStyle"
			}
		);
        
		// For UE versions before 5.5, add the StructUtils dependency 
		if (Target.Version.MajorVersion < 5 || (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion < 5))
		{
			PrivateDependencyModuleNames.Add("StructUtils");
		}
        
		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}