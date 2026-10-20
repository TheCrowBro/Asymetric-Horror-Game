using UnrealBuildTool;
using System.Collections.Generic;

public class MansionEditorTarget : TargetRules
{
	public MansionEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("Mansion");
	}
}
