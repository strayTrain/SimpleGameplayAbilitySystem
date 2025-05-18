using UnrealBuildTool;

public class SimpleGameplayAbilitySystemTest : ModuleRules
{
    public SimpleGameplayAbilitySystemTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "SimpleGameplayAbilitySystem",
            "CoreUObject",
            "UnrealEd",
            "AssetTools",
            "Engine",
            "GameplayTags", "GameplayTagsEditor"
        });
        
        if (Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
            OptimizeCode = CodeOptimization.Never;
        }
    }
}