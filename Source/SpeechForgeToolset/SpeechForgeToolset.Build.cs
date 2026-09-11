using UnrealBuildTool;

public class SpeechForgeToolset : ModuleRules
{
	public SpeechForgeToolset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ToolsetRegistry",  // UToolsetDefinition and UAgentSkill are public base classes here
				"SpeechForge",      // the capability this adapter exposes, and the types in its signatures
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",         // IPluginManager, so GetToolsetVersion() reads the descriptor
				"UnrealEd",         // GEditor, to reach the editor subsystem
			}
			);

		// Deliberately no dependency on ModelContextProtocol itself. Tools are registered with
		// ToolsetRegistry, and MCP picks them up from there - going direct would couple this module
		// to a transport it does not care about.
	}
}
