using UnrealBuildTool;
public class AnEmberRemains : ModuleRules
{
    public AnEmberRemains(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs; bUseUnity=false;
        PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","InputCore"});
        PrivateDependencyModuleNames.AddRange(new string[]{"RenderCore","RHI","Niagara"});
        if(Target.bBuildEditor) {
            PrivateDependencyModuleNames.AddRange(new string[]{"UnrealEd","MaterialEditor","NiagaraShader"});
            PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory,"Plugins/FX/Niagara/Source/Niagara/Internal"));
            PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory,"Plugins/FX/Niagara/Source/NiagaraShader/Internal"));
        }
    }
}
