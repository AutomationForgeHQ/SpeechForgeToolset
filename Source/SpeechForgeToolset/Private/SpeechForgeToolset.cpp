#include "SpeechForgeToolset.h"

#include "SpeechForgeToolsetModule.h"
#include "SpeechForgeAsyncResult.h"
#include "SpeechForge.h"
#include "SpeechForgeSubsystem.h"
#include "SpeechForgeSettings.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Containers/Ticker.h"
#include "Editor.h"

FString USpeechForgeToolset::ApplyRecordedAudio(
	const FString& AssetPath, FName LineId, const FString& AudioSource)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return TEXT("ERROR: the SpeechForge subsystem is not available.");
	}

	FString Error;
	const FString Applied = Subsystem->ApplyRecordedAudio(AssetPath, LineId, AudioSource, Error);
	return Applied.IsEmpty() ? FString::Printf(TEXT("ERROR: %s"), *Error) : Applied;
}

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

FString USpeechForgeToolset::OpenSpeechLibrary(const FString& BankPath, const FString& LineId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FString();
	}

	Subsystem->OpenLibraryAt(BankPath, LineId.IsEmpty() ? NAME_None : FName(*LineId));

	return LineId.IsEmpty()
		? FString::Printf(TEXT("Showing %s in the Speech Library."), *BankPath)
		: FString::Printf(TEXT("Showing '%s' of %s in the Speech Library."), *LineId, *BankPath);
}

FString USpeechForgeToolset::GetSpeechBankSource(const FString& BankPath)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->GetBankSourceDescription(BankPath) : FString();
}

TArray<FSpeechSourceDrift> USpeechForgeToolset::CheckSpeechSourceDrift()
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->CheckSourceDrift() : TArray<FSpeechSourceDrift>();
}

FSpeechCostEstimate USpeechForgeToolset::EstimateSpeechCost(
	const TArray<FSpeechLineHandle>& Handles, bool IncludeCurrent)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->EstimateGenerationCost(Handles, IncludeCurrent) : FSpeechCostEstimate();
}

FSpeechCostEstimate USpeechForgeToolset::EstimateSpeechConversionCost(
	const TArray<FSpeechLineHandle>& Handles, const FString& SourceAudio)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->EstimateConversionCost(Handles, SourceAudio) : FSpeechCostEstimate();
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

int32 USpeechForgeToolset::RemoveSpeechLines(const FString& BankPath, const TArray<FName>& LineIds)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return 0;
	}

	FString Error;
	const int32 Removed = Subsystem->RemoveBankLines(BankPath, LineIds, Error);

	if (!Error.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(*Error);
	}

	return Removed;
}

int32 USpeechForgeToolset::ClearSpeechBank(const FString& BankPath)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return 0;
	}

	FString Error;
	const int32 Removed = Subsystem->ClearBankLines(BankPath, Error);

	if (!Error.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(*Error);
	}

	return Removed;
}

FString USpeechForgeToolset::CreateVoiceProfile(
	const FString& AssetPath,
	const FString& DisplayName,
	const FString& ProviderVoiceId,
	const FString& ProviderVoiceName,
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
			TEXT("A voice profile needs a Provider Voice Id. Call List Provider Voices to find one."));
		return FString();
	}

	return Subsystem->CreateVoiceProfile(
		AssetPath, DisplayName, ProviderVoiceId, ProviderVoiceName, ProviderId, ModelId, Provenance);
}

FString USpeechForgeToolset::CreateOrUpdateSpeaker(
	FName SpeakerId,
	const FString& DisplayName,
	const FString& Description,
	const FString& VoiceProfilePath)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FString();
	}

	if (SpeakerId.IsNone())
	{
		UKismetSystemLibrary::RaiseScriptError(
			TEXT("A speaker needs a Speaker Id - it is the key every line carries."));
		return FString();
	}

	const FString Path = Subsystem->CreateOrUpdateSpeaker(SpeakerId, DisplayName, Description, VoiceProfilePath);
	if (Path.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Could not create or update the speaker."));
	}
	return Path;
}

TArray<FString> USpeechForgeToolset::ListSpeakers()
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->FindSpeakerAssets() : TArray<FString>();
}

TArray<FString> USpeechForgeToolset::ListVoiceProfiles()
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->FindVoiceProfiles() : TArray<FString>();
}

void USpeechForgeToolset::SetSpeechLineVoiceOverride(
	const FSpeechLineHandle& Handle, const FString& VoiceProfilePath)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (Subsystem && !Subsystem->SetLineVoiceOverride(Handle, VoiceProfilePath))
	{
		UKismetSystemLibrary::RaiseScriptError(*FString::Printf(
			TEXT("No line at '%s'."), *Handle.ToString()));
	}
}

void USpeechForgeToolset::UpdateSpeechLine(
	const FSpeechLineHandle& Handle,
	const FString& Text,
	const FString& Direction,
	FName SpeakerId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (Subsystem && !Subsystem->UpdateLineAuthoring(Handle, Text, Direction, SpeakerId))
	{
		UKismetSystemLibrary::RaiseScriptError(*FString::Printf(
			TEXT("No line at '%s'."), *Handle.ToString()));
	}
}

UToolCallAsyncResultSpeechString* USpeechForgeToolset::GenerateSpeechTake(const FSpeechLineHandle& Handle)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return nullptr;
	}

	UToolCallAsyncResultSpeechString* Async = NewObject<UToolCallAsyncResultSpeechString>();

	Subsystem->GenerateLineTake(Handle,
		[Async](bool bSuccess, const FSpeechLineTake& Take, const FString& Error)
	{
		if (bSuccess)
		{
			Async->SetValue(FString::Printf(
				TEXT("Take %s: %s (%.2fs, %d characters billed). The line is untouched; Apply Speech Take makes this the line."),
				*Take.TakeId.ToString(), *Take.SoundPath, Take.DurationSeconds, Take.BilledCharacters));
		}
		else
		{
			Async->SetError(Error);
		}
	});

	return Async;
}

TArray<FSpeechLineTake> USpeechForgeToolset::ListSpeechTakes(const FSpeechLineHandle& Handle)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->GetLineTakes(Handle) : TArray<FSpeechLineTake>();
}

FString USpeechForgeToolset::ApplySpeechTake(const FSpeechLineHandle& Handle, FName TakeId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return TEXT("ERROR: the editor is not up.");
	}

	FString Error;
	if (!Subsystem->ApplyLineTake(Handle, TakeId, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(*Error);
		return FString::Printf(TEXT("ERROR: %s"), *Error);
	}

	return FString::Printf(TEXT("Take %s is now line '%s'."),
		*TakeId.ToString(), *Handle.LineId.ToString());
}

TArray<FString> USpeechForgeToolset::ListSpeechTakeRows(const FString& AssetPath, const FString& LineId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return {};
	}

	const FSpeechLineHandle Handle(AssetPath, FName(*LineId));

	// What the line actually plays, which is the only honest basis for saying a take is in use.
	// The stored chosen-take id records a decision, and a decision can be out of date: a session
	// that keeps no takes applies a recording straight to the line without a ledger row, and the
	// previously chosen take then went on claiming to be the one you were hearing.
	const FString LineSound = GetSpeechLineSoundPath(AssetPath, LineId);

	TArray<FString> Rows;
	for (const FSpeechLineTake& Take : Subsystem->GetLineTakes(Handle))
	{
		// Variants ride the same row rather than needing a second call: the reader is a panel in
		// another plugin that already parses this, and a take with no re-voicings adds one empty
		// field. Separators are stripped from labels so a voice named with a comma cannot split a
		// row - the transport is positional text by construction, not a format anyone should trust
		// with arbitrary strings.
		FString VariantField;
		for (const FSpeechTakeAudio& Variant : Take.Variants)
		{
			FString Label = Variant.Label;
			for (const TCHAR* Separator : { TEXT(","), TEXT("|"), TEXT("~"), TEXT(";") })
			{
				Label.ReplaceInline(Separator, TEXT(" "));
			}

			VariantField += FString::Printf(TEXT("%s~%s~%s~%.2f;"),
				*Variant.VariantId.ToString(), *Label, *Variant.SoundPath, Variant.DurationSeconds);
		}

		// In use means the line is playing this take's audio - its own recording, or one of its
		// voices. Anything else is a claim nobody checked.
		bool bIsLineAudio = !LineSound.IsEmpty() && Take.SoundPath == LineSound;
		if (!bIsLineAudio)
		{
			for (const FSpeechTakeAudio& Variant : Take.Variants)
			{
				if (!LineSound.IsEmpty() && Variant.SoundPath == LineSound)
				{
					bIsLineAudio = true;
					break;
				}
			}
		}

		Rows.Add(FString::Printf(TEXT("%s|%s|%.2f|%s|%s|%s|%d|%s|%s"),
			*Take.TakeId.ToString(),
			Take.Kind == ESpeechTakeKind::Recorded ? TEXT("Recorded") : TEXT("Generated"),
			Take.DurationSeconds,
			*Take.CreatedAt.ToString(TEXT("%Y-%m-%d %H:%M")),
			*Take.SoundPath,
			*Take.TakeDir,
			bIsLineAudio ? 1 : 0,
			Take.ChosenVariantId.IsNone() ? TEXT("") : *Take.ChosenVariantId.ToString(),
			*VariantField));
	}
	return Rows;
}

FString USpeechForgeToolset::ApplySpeechTakeByPath(
	const FString& AssetPath, const FString& LineId, const FString& TakeId)
{
	return ApplySpeechTake(FSpeechLineHandle(AssetPath, FName(*LineId)), FName(*TakeId));
}

UToolCallAsyncResultSpeechString* USpeechForgeToolset::GenerateSpeechTakeByPath(
	const FString& AssetPath, const FString& LineId)
{
	return GenerateSpeechTake(FSpeechLineHandle(AssetPath, FName(*LineId)));
}

FString USpeechForgeToolset::RegisterRecordedSpeechTake(
	const FString& AssetPath,
	const FString& LineId,
	const FString& WavPath,
	const FString& TakeDir)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FString();
	}

	FString Error;
	const FString TakeId = Subsystem->RegisterRecordedTake(
		FSpeechLineHandle(AssetPath, FName(*LineId)), WavPath, TakeDir, Error);

	if (TakeId.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(*Error);
	}
	return TakeId;
}

void USpeechForgeToolset::MarkSpeechTakeChosen(const FString& AssetPath, const FString& LineId, const FString& TakeId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (Subsystem &&
		!Subsystem->MarkTakeChosen(FSpeechLineHandle(AssetPath, FName(*LineId)), FName(*TakeId)))
	{
		UKismetSystemLibrary::RaiseScriptError(*FString::Printf(
			TEXT("No take '%s' on line '%s'."), *TakeId, *LineId));
	}
}

FString USpeechForgeToolset::GetSpeechLineSoundPath(const FString& AssetPath, const FString& LineId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FString();
	}

	const TArray<FSpeechLineStatus> Status =
		Subsystem->GetLineStatus({ FSpeechLineHandle(AssetPath, FName(*LineId)) });
	return Status.Num() > 0 ? Status[0].SoundPath : FString();
}

UToolCallAsyncResultSpeechString* USpeechForgeToolset::ConvertSpeechLine(
	const FSpeechLineHandle& Handle, const FString& SourceAudio)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return nullptr;
	}

	UToolCallAsyncResultSpeechString* Async = NewObject<UToolCallAsyncResultSpeechString>();

	Subsystem->ConvertLineAudio(Handle, SourceAudio,
		[Async](bool bSuccess, const FString& Message)
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

TArray<FString> USpeechForgeToolset::ListSpeechTranslationProviders()
{
	TArray<FString> Ids;
	if (FSpeechForgeModule* Module = FSpeechForgeModule::GetPtr())
	{
		for (const FName Id : Module->GetTranslationProviderIds())
		{
			Ids.Add(Id.ToString());
		}
	}
	return Ids;
}

UToolCallAsyncResultSpeechString* USpeechForgeToolset::LocalizeSpeechBank(
	const FString& BankPath,
	const FString& TargetLanguage,
	const FString& TranslationProviderId,
	bool Force)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return nullptr;
	}

	UToolCallAsyncResultSpeechString* Async = NewObject<UToolCallAsyncResultSpeechString>();

	const FString Refusal = Subsystem->LocalizeBank(
		BankPath, TargetLanguage,
		TranslationProviderId.IsEmpty() ? NAME_None : FName(*TranslationProviderId),
		Force,
		[Async](bool bSuccess, const FString& Message)
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

	if (!Refusal.IsEmpty())
	{
		Async->SetError(Refusal);
	}

	return Async;
}

TArray<FSpeechLocalizationStatus> USpeechForgeToolset::GetSpeechLocalizationStatus(const FString& BankPath)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	return Subsystem ? Subsystem->GetLocalizationStatus(BankPath) : TArray<FSpeechLocalizationStatus>();
}

UToolCallAsyncResultSpeechString* USpeechForgeToolset::DubSpeechLine(const FSpeechLineHandle& Handle)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return nullptr;
	}

	UToolCallAsyncResultSpeechString* Async = NewObject<UToolCallAsyncResultSpeechString>();

	Subsystem->DubLineAudio(Handle,
		[Async](bool bSuccess, const FString& Message)
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

UToolCallAsyncResultSpeechString* USpeechForgeToolset::ConvertSpeechLineByPath(
	const FString& AssetPath, const FString& LineId, const FString& SourceAudio)
{
	return ConvertSpeechLine(FSpeechLineHandle(AssetPath, FName(*LineId)), SourceAudio);
}

UToolCallAsyncResultSpeechString* USpeechForgeToolset::ConvertSpeechTake(
	const FSpeechLineHandle& Handle, FName TakeId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return nullptr;
	}

	UToolCallAsyncResultSpeechString* Async = NewObject<UToolCallAsyncResultSpeechString>();

	Subsystem->ConvertTakeAudio(Handle, TakeId,
		[Async](bool bSuccess, const FString& Message)
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

UToolCallAsyncResultSpeechString* USpeechForgeToolset::ConvertSpeechTakeByPath(
	const FString& AssetPath, const FString& LineId, const FString& TakeId)
{
	return ConvertSpeechTake(FSpeechLineHandle(AssetPath, FName(*LineId)), FName(*TakeId));
}

FString USpeechForgeToolset::RemoveSpeechTake(
	const FString& AssetPath, const FString& LineId, const FString& TakeId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FString();
	}

	FString Error;
	if (!Subsystem->RemoveTake(FSpeechLineHandle(AssetPath, FName(*LineId)), FName(*TakeId), Error))
	{
		UKismetSystemLibrary::RaiseScriptError(*Error);
		return FString();
	}

	return FString::Printf(TEXT("Take %s removed from '%s'."), *TakeId, *LineId);
}

FString USpeechForgeToolset::RemoveSpeechTakeVariant(
	const FString& AssetPath, const FString& LineId, const FString& TakeId, const FString& VariantId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FString();
	}

	FString Error;
	if (!Subsystem->RemoveTakeVariant(
		FSpeechLineHandle(AssetPath, FName(*LineId)), FName(*TakeId), FName(*VariantId), Error))
	{
		UKismetSystemLibrary::RaiseScriptError(*Error);
		return FString();
	}

	return FString::Printf(TEXT("Dropped %s from take %s. Its audio is untouched."), *VariantId, *TakeId);
}

FString USpeechForgeToolset::SetSpeechTakeVariant(
	const FString& AssetPath, const FString& LineId, const FString& TakeId, const FString& VariantId)
{
	USpeechForgeSubsystem* Subsystem = GetSubsystemChecked();
	if (!Subsystem)
	{
		return FString();
	}

	FString Error;
	const bool bSet = Subsystem->SetTakeVariant(
		FSpeechLineHandle(AssetPath, FName(*LineId)),
		FName(*TakeId),
		VariantId.IsEmpty() ? NAME_None : FName(*VariantId),
		Error);

	if (!bSet)
	{
		UKismetSystemLibrary::RaiseScriptError(*Error);
		return FString();
	}

	return VariantId.IsEmpty()
		? FString::Printf(TEXT("Take %s plays its own recording again."), *TakeId)
		: FString::Printf(TEXT("Take %s will be heard as %s."), *TakeId, *VariantId);
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
			TEXT("'%s' resolves to no voice. Cast its speaker (Create Or Update Speaker with a voice ")
			TEXT("profile), set a line override, or set the asset's default."),
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
