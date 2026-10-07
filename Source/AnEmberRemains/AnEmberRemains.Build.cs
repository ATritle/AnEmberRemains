using UnrealBuildTool;
public class AnEmberRemains : ModuleRules
{
    public AnEmberRemains(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs; bUseUnity=false;
        PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","InputCore"});
        PrivateDependencyModuleNames.AddRange(new string[]{"RenderCore","RHI"});
    }
}
