using UnrealBuildTool;
public class AnEmberRemainsTarget : TargetRules
{
    public AnEmberRemainsTarget(TargetInfo Target) : base(Target) {
        Type=TargetType.Game; DefaultBuildSettings=BuildSettingsVersion.Latest;
        IncludeOrderVersion=EngineIncludeOrderVersion.Latest; ExtraModuleNames.Add("AnEmberRemains");
    }
}
