using UnrealBuildTool;

public class AGPAcceptanceTools : ModuleRules
{
	public AGPAcceptanceTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Landscape" });
		PrivateDependencyModuleNames.AddRange(new[] { "Json" });
	}
}
