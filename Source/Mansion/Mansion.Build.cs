using UnrealBuildTool;

public class Mansion : ModuleRules
{
	public Mansion(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets source files include each other as "Core/...", "Survivor/..." etc.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"DeveloperSettings",
			"NetCore",
			"AIModule",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});
	}
}
