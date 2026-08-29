#include "SpeechForgeToolsetModule.h"

#include "SpeechForgeToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

DEFINE_LOG_CATEGORY(LogSpeechForgeToolset);

#define LOCTEXT_NAMESPACE "FSpeechForgeToolsetModule"

void FSpeechForgeToolsetModule::StartupModule()
{
	if (!UToolsetRegistry::IsAvailable())
	{
		// Expected outside the editor, and whenever the experimental plugins are off. Not an error.
		UE_LOG(LogSpeechForgeToolset, Log,
			TEXT("Toolset registry unavailable - SpeechForge tools not registered."));
		return;
	}

	if (UToolsetRegistry::IsToolsetClassRegistered(USpeechForgeToolset::StaticClass()))
	{
		bRegistered = true;
		return;
	}

	UToolsetRegistry::RegisterToolsetClass(USpeechForgeToolset::StaticClass());
	bRegistered = UToolsetRegistry::IsToolsetClassRegistered(USpeechForgeToolset::StaticClass());

	UE_LOG(LogSpeechForgeToolset, Log, TEXT("SpeechForge toolset %s."),
		bRegistered ? TEXT("registered") : TEXT("failed to register"));
}

void FSpeechForgeToolsetModule::ShutdownModule()
{
	if (bRegistered && UToolsetRegistry::IsAvailable())
	{
		UToolsetRegistry::UnregisterToolsetClass(USpeechForgeToolset::StaticClass());
		bRegistered = false;
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSpeechForgeToolsetModule, SpeechForgeToolset)
