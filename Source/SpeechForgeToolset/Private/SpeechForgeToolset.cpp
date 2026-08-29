#include "SpeechForgeToolset.h"

#include "SpeechForgeToolsetModule.h"
#include "SpeechForgeAsyncResult.h"
#include "SpeechForgeSubsystem.h"
#include "SpeechForgeSettings.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Containers/Ticker.h"
#include "Editor.h"

USpeechForgeSubsystem* USpeechForgeToolset::GetSubsystemChecked()
{
	USpeechForgeSubsystem* Subsystem = USpeechForgeSubsystem::Get();
	if (!Subsystem)
	{
		UKismetSystemLibrary::RaiseScriptError(
			TEXT("SpeechForge is not available. It is an editor-only plugin and needs the editor running."));
	}
	return Subsystem;
}

// -------------------------------------------------------------------------------------------------
// Discovery
// -------------------------------------------------------------------------------------------------

TArray<FString> USpeechForgeToolset::ListSpeechAssets()
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->FindSpeechAssets() : TArray<FString>();
}

FSpeechOutputPaths USpeechForgeToolset::GetSpeechOutputPaths()
{
	return USpeechForgeSettings::Get()->GetOutputPaths();
}

FSpeechOutputPaths USpeechForgeToolset::SetSpeechOutputRoot(const FString& ContentPath)
{
	const FSpeechOutputPaths Paths = USpeechForgeSettings::SetOutputRoot(ContentPath);

	if (!Paths.Problem.IsEmpty())
	{
		// A refused root is a mistake the agent can correct, so raise it rather than returning a
		// struct whose Problem field it may not read.
		UKismetSystemLibrary::RaiseScriptError(Paths.Problem);
	}

	return Paths;
}

TArray<FSpeechLineStatus> USpeechForgeToolset::GetSpeechLineStatus(const TArray<FSpeechLineHandle>& Handles)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->GetLineStatus(Handles) : TArray<FSpeechLineStatus>();
}

TArray<FName> USpeechForgeToolset::ListSpeechProviders()
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->GetProviderIds() : TArray<FName>();
}

FSpeechProviderCaps USpeechForgeToolset::GetSpeechProviderCapabilities(FName ProviderId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FSpeechProviderCaps();
	}

	const FSpeechProviderCaps Caps = Subsystem->GetProviderCaps(ProviderId);
	if (Caps.ProviderId.IsNone())
	{
		UKismetSystemLibrary::RaiseScriptError(*FString::Printf(
			TEXT("No provider named '%s'. Call List Speech Providers to see what exists."),
			*ProviderId.ToString()));
	}

	return Caps;
}

FSpeechCredentialInfo USpeechForgeToolset::GetSpeechCredentialStatus(FName ProviderId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->GetCredentialInfo(ProviderId) : FSpeechCredentialInfo();
}

UToolCallAsyncResultSpeechString* USpeechForgeToolset::TestSpeechProviderConnection(FName ProviderId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return nullptr;
	}

	UToolCallAsyncResultSpeechString* Async = NewObject<UToolCallAsyncResultSpeechString>();

	Subsystem->TestConnectionAsync(ProviderId, [Async](bool bSuccess, const FString& Message)
	{
		if (bSuccess)
		{
			Async->SetValue(Message);
		}
		else
		{
			Async->SetError(Message);
		}
	});

	return Async;
}

UToolCallAsyncResultSpeechVoices* USpeechForgeToolset::ListProviderVoices(FName ProviderId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return nullptr;
	}

	UToolCallAsyncResultSpeechVoices* Async = NewObject<UToolCallAsyncResultSpeechVoices>();

	Subsystem->ListProviderVoices(ProviderId,
		[Async](bool bSuccess, const TArray<FSpeechRemoteVoiceInfo>& Voices, const FString& Error)
	{
		if (bSuccess)
		{
			Async->SetValue(Voices);
		}
		else
		{
			Async->SetError(Error);
		}
	});

	return Async;
}

// -------------------------------------------------------------------------------------------------
// Cost
// -------------------------------------------------------------------------------------------------

FSpeechCostEstimate USpeechForgeToolset::EstimateSpeechCost(
	const TArray<FSpeechLineHandle>& Handles, bool IncludeCurrent)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->EstimateGenerationCost(Handles, IncludeCurrent) : FSpeechCostEstimate();
}

// -------------------------------------------------------------------------------------------------
// Authoring
// -------------------------------------------------------------------------------------------------

FString USpeechForgeToolset::CreateOrUpdateSpeechBank(
	const FString& AssetPath,
	const FString& BankName,
	const TArray<FSpeechLineSpec>& Lines,
	FName DefaultSpeakerId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FString();
	}

	const FString Path = Subsystem->CreateOrUpdateBank(AssetPath, BankName, Lines, DefaultSpeakerId);
	if (Path.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(
			TEXT("Could not create or update the bank. Check the asset path is a valid content path."));
	}

	return Path;
}

FString USpeechForgeToolset::CreateSpeechVoice(
	const FString& AssetPath,
	FName SpeakerId,
	const FString& ProviderVoiceId,
	FName ProviderId,
	const FString& ModelId,
	ESpeechVoiceProvenance Provenance)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FString();
	}

	if (ProviderVoiceId.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(
			TEXT("A voice needs a Provider Voice Id. Call List Provider Voices to find one."));
		return FString();
	}

	return Subsystem->CreateVoice(AssetPath, SpeakerId, ProviderVoiceId, ProviderId, ModelId, Provenance);
}

FSpeechVoiceResolution USpeechForgeToolset::ResolveSpeechVoice(const FSpeechLineHandle& Handle)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FSpeechVoiceResolution();
	}

	const FSpeechVoiceResolution Resolution = Subsystem->ResolveVoice(Handle);
	if (!Resolution.IsValid())
	{
		UKismetSystemLibrary::RaiseScriptError(*FString::Printf(
			TEXT("'%s' resolves to no voice. Set one on the line, on the asset's defaults, or create ")
			TEXT("a Speech Voice for its speaker."),
			*Handle.ToString()));
	}

	return Resolution;
}

// -------------------------------------------------------------------------------------------------
// Pipeline
// -------------------------------------------------------------------------------------------------

UToolCallAsyncResultSpeechStatus* USpeechForgeToolset::WatchBatch(
	const FString& BatchId, const TArray<FSpeechLineHandle>& Handles)
{
	UToolCallAsyncResultSpeechStatus* Async = NewObject<UToolCallAsyncResultSpeechStatus>();

	// Handles are captured rather than read back from the batch: a finished batch is dropped from
	// tracking, so by the time it reports done it no longer knows what it contained.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[Async, BatchId, Handles](float) -> bool
	{
		USpeechForgeSubsystem* Subsystem = USpeechForgeSubsystem::Get();
		if (!Subsystem)
		{
			Async->SetError(TEXT("SpeechForge went away while the batch was running."));
			return false;
		}

		const FSpeechBatchStatus Status = Subsystem->GetBatchStatus(BatchId);
		if (Status.bTracked && !Status.bFinished)
		{
			return true;
		}

		Async->SetValue(Subsystem->GetLineStatus(Handles));
		return false;
	}), 1.0f);

	return Async;
}

UToolCallAsyncResultSpeechStatus* USpeechForgeToolset::GenerateSpeech(
	const TArray<FSpeechLineHandle>& Handles, bool Force)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return nullptr;
	}

	const FString BatchId = Subsystem->GenerateLines(Handles, Force);

	if (BatchId.IsEmpty())
	{
		// Not an error. Every line was current, in flight, or graduated - which is the normal answer
		// to running the same generate twice, and the answer that keeps a retry from paying twice.
		UToolCallAsyncResultSpeechStatus* Immediate = NewObject<UToolCallAsyncResultSpeechStatus>();
		Immediate->SetValue(Subsystem->GetLineStatus(Handles));
		return Immediate;
	}

	return WatchBatch(BatchId, Handles);
}

void USpeechForgeToolset::RefetchSpeechLine(const FSpeechLineHandle& Handle)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (Subsystem && !Subsystem->RefetchLine(Handle))
	{
		UKismetSystemLibrary::RaiseScriptError(*FString::Printf(
			TEXT("Could not re-fetch '%s'. It may have no provider request id, or its provider may ")
			TEXT("not keep a history. Check Get Speech Line Status."),
			*Handle.ToString()));
	}
}

void USpeechForgeToolset::AcceptCurrentSpeechAudio(const FSpeechLineHandle& Handle)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (Subsystem && !Subsystem->AcceptCurrentAudio(Handle))
	{
		UKismetSystemLibrary::RaiseScriptError(*FString::Printf(
			TEXT("Could not accept '%s'. Only a line that has generated audio can be accepted."),
			*Handle.ToString()));
	}
}

void USpeechForgeToolset::MarkSpeechLineRecorded(
	const FSpeechLineHandle& Handle, const FString& RecordedSoundPath)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (Subsystem && !Subsystem->MarkRecorded(Handle, RecordedSoundPath))
	{
		UKismetSystemLibrary::RaiseScriptError(*FString::Printf(
			TEXT("Could not point '%s' at '%s'. Check the line exists and the sound path is a real asset."),
			*Handle.ToString(), *RecordedSoundPath));
	}
}

int32 USpeechForgeToolset::DetectEditedSpeechAudio(const TArray<FSpeechLineHandle>& Handles)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->DetectEditedAudio(Handles) : 0;
}
