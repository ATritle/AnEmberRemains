using UnrealBuildTool;
public class AnEmberRemainsEditorTarget : TargetRules
{
    public AnEmberRemainsEditorTarget(TargetInfo Target) : base(Target) {
        Type=TargetType.Editor; DefaultBuildSettings=BuildSettingsVersion.Latest;
        IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("AnEmberRemains");
    }
}
