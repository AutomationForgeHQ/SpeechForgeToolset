#pragma once

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

SPEECHFORGETOOLSET_API DECLARE_LOG_CATEGORY_EXTERN(LogSpeechForgeToolset, Log, All);

/**
 * Registers the SpeechForge toolset with the registry.
 *
 * **This one line of registration is the whole reason this module exists.** Native `UAgentSkill`
 * classes are auto-discovered; toolsets are not. Forgetting to register makes the entire toolset
 * invisible, with no warning anywhere - the tools simply are not in the list, which reads exactly
 * like the plugin being disabled.
 */
class FSpeechForgeToolsetModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	bool bRegistered = false;
};
