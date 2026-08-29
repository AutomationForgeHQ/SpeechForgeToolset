// SpeechForge, as tools an agent can call.

#pragma once

#include "CoreMinimal.h"
#include "SpeechForgeTypes.h"
#include "SpeechLine.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "SpeechForgeToolset.generated.h"

class UToolCallAsyncResultSpeechStatus;
class UToolCallAsyncResultSpeechVoices;
class UToolCallAsyncResultSpeechString;

/**
 * The SpeechForge pipeline exposed as Model Context Protocol tools.
 *
 * Every function forwards to USpeechForgeSubsystem and adds nothing. All logic, state and safety
 * live in the capability plugin; this is a surface, and deleting it changes nothing about how
 * SpeechForge behaves.
 *
 * Signatures are the schema. Parameters and return values are USTRUCTs and enums rather than JSON
 * strings, so the registry publishes a typed schema an agent can fill in correctly without guessing
 * field names, and doc comments here become the tool descriptions it reads. They are written for
 * that reader, not for us.
 *
 * Failures raise a script error rather than returning an ok/error envelope. The registry turns those
 * into tool errors the agent can act on.
 *
 * Two things are deliberately absent. There is no tool for setting an API key - an agent that can
 * write secrets into the OS credential vault is a liability with no matching benefit, and signing in
 * is a human action performed once in Project Settings. And there is no tool that creates or deletes
 * a voice on the provider: a key that can create one can generally delete one, and deleting a voice
 * loses every line it ever spoke, unrecoverably.
 */
UCLASS(BlueprintType)
class SPEECHFORGETOOLSET_API USpeechForgeToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:

	virtual FString GetToolsetVersion() const override { return TEXT("0.1"); }

	// ---------------------------------------------------------------------------------------------
	// Discovery
	// ---------------------------------------------------------------------------------------------

	/**
	 * List the content paths of speech banks and single-line speech assets in the project.
	 *
	 * @return Asset paths. Every other tool here takes these as part of a line handle.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Discovery")
	static TArray<FString> ListSpeechAssets();

	// ---------------------------------------------------------------------------------------------
	// Output
	// ---------------------------------------------------------------------------------------------

	/**
	 * Where the pipeline writes: the configured root, and every folder derived from it.
	 *
	 * Worth reading before generating anything into a project you did not set up, because it is the
	 * only way to know where banks, voices and audio will land without waiting to see where they
	 * landed.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Output")
	static FSpeechOutputPaths GetSpeechOutputPaths();

	/**
	 * Point the pipeline at a different content root.
	 *
	 * Only the root is settable. Banks, Voices and Sounds beneath it are derived and stay as they are,
	 * because the sort by kind is how the output stays readable after the second run rather than a
	 * preference to be overridden.
	 *
	 * **Moves nothing.** A bank already written keeps working from wherever it is, and its lines keep
	 * pointing at the audio they already have; this decides where the next assets are created.
	 *
	 * The change is written to the project's DefaultEditor.ini and persists across restarts.
	 *
	 * @param ContentPath A content folder such as `/Game/_EP1/Speech`. Must be under a mounted root -
	 *        content written somewhere unmounted is created in memory, never saved, and reported as a
	 *        success.
	 * @return The resolved structure. On refusal, `Problem` says why and every path describes what is
	 *         still configured rather than what was asked for.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Output")
	static FSpeechOutputPaths SetSpeechOutputRoot(const FString& ContentPath);

	/**
	 * Report status, origin, staleness and resolved voice for speech lines.
	 *
	 * Three separate facts come back per line and they are not interchangeable. Status is where the
	 * line is in the pipeline. Origin is where its audio came from. Stale means the audio no longer
	 * matches what the line now says.
	 *
	 * **The combination worth acting on is Stale with an Origin of Recorded.** That means the script
	 * was rewritten after an actor recorded the line, so it needs a pickup session rather than a
	 * regeneration - and regenerating it would throw away a real performance. Stale Reason says which
	 * case it is in words.
	 *
	 * @param Handles Lines to report on. A handle with no Line Id means every line in that asset.
	 *        An empty array means every line in the project.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Discovery")
	static TArray<FSpeechLineStatus> GetSpeechLineStatus(const TArray<FSpeechLineHandle>& Handles);

	/** List the speech providers this project has compiled in. */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Discovery")
	static TArray<FName> ListSpeechProviders();

	/**
	 * Report what a provider can actually do: sample rate, whether it charges, whether it seeds,
	 * whether it returns timings, how many requests it allows at once.
	 *
	 * **Read this rather than assuming.** Two fields in particular change what is safe to do. Seed Is
	 * Best Effort being true means a regenerated line will not be identical to the one it replaced,
	 * so regenerating a whole bank subtly re-performs every line in it. Max Concurrent Requests is a
	 * property of the account rather than the code, and exceeding it does not error - it queues.
	 *
	 * Setup Hint is the field to act on when it is not empty: it names something a human has to do,
	 * which is never something you can do for them.
	 *
	 * @param ProviderId A name from List Speech Providers. Leave empty for the default provider.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Discovery")
	static FSpeechProviderCaps GetSpeechProviderCapabilities(FName ProviderId);

	/**
	 * Report whether a provider has a usable API key, and where it is read from.
	 *
	 * Never returns the key itself, and providers cannot be signed in to from here. When Configured
	 * is false, tell the user to add the key in Project Settings rather than trying to set it.
	 *
	 * @param ProviderId A name from List Speech Providers. Leave empty for the default provider.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Discovery")
	static FSpeechCredentialInfo GetSpeechCredentialStatus(FName ProviderId);

	/**
	 * Make one cheap authenticated call to a provider and report what came back.
	 *
	 * Costs nothing and generates nothing. Worth doing once before a first batch, and it is the
	 * fastest way to tell a wrong key apart from a wrong voice id when generation fails. Reports the
	 * account tier and remaining credits, which is also the honest answer to "can we afford this".
	 *
	 * @param ProviderId A name from List Speech Providers. Leave empty for the default provider.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Discovery")
	static UToolCallAsyncResultSpeechString* TestSpeechProviderConnection(FName ProviderId);

	/**
	 * List the voices this account holds on a provider.
	 *
	 * Costs nothing. Use it to find a Provider Voice Id before creating a Speech Voice asset.
	 *
	 * **Check Is Premade before choosing one.** A premade voice belongs to the provider rather than
	 * to this account, and providers retire them - which would take every line generated against it.
	 * For anything that has to outlive a prototype, tell the user to design or clone a voice of their
	 * own first, in the provider's own interface.
	 *
	 * @param ProviderId A name from List Speech Providers. Leave empty for the default provider.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Discovery")
	static UToolCallAsyncResultSpeechVoices* ListProviderVoices(FName ProviderId);

	// ---------------------------------------------------------------------------------------------
	// Cost
	// ---------------------------------------------------------------------------------------------

	/**
	 * Price what generating these lines would cost, before anything is sent.
	 *
	 * **Call this before Generate Speech and tell the user the number.** Billing is per character of
	 * input and it is charged the instant a request is made, so a failed generation is still a paid
	 * one and nothing refunds a take that turns out badly.
	 *
	 * Unlike most such estimates this is exact rather than projected, and needs no network call.
	 * Skipped Count is how many of the lines asked about would not actually regenerate because they
	 * are already current, in flight, or graduated.
	 *
	 * @param Handles Lines to price. A handle with no Line Id means every line in that asset.
	 * @param IncludeCurrent Price everything, including lines that would not regenerate. Leave false
	 *        to get the number that will actually be spent.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Cost")
	static FSpeechCostEstimate EstimateSpeechCost(
		const TArray<FSpeechLineHandle>& Handles,
		bool IncludeCurrent);

	// ---------------------------------------------------------------------------------------------
	// Authoring
	// ---------------------------------------------------------------------------------------------

	/**
	 * Create a speech bank, or add lines to one that already exists.
	 *
	 * Writing the line is most of the job, and two fields do different work. Text is what the player
	 * reads and what is spoken. **Direction is performance notes and must be kept out of Text** - put
	 * it in the text and the subtitle reads "[sighs] ...damn it." on screen. The audio will be
	 * perfect and the asset will be correct and only somebody looking at the screen will ever catch
	 * it.
	 *
	 * Direction works as short performance words: whispers, sarcastic, exhausted, shouting. It only
	 * lands on models that take inline direction, and only when the voice's stability leaves room for
	 * it - a very stable voice is deaf to it by design.
	 *
	 * Set Previous Line Id on lines that follow one another in a conversation. Providers that support
	 * stitching are then given the preceding line, so prosody carries across the exchange instead of
	 * every line sounding recorded in a separate session. It costs nothing and it is the difference
	 * between a conversation and a list of sentences.
	 *
	 * A line whose id already exists is updated rather than duplicated, and keeps the audio it has
	 * already been paid for.
	 *
	 * @param AssetPath Content path of an existing bank. Leave empty to create one under the
	 *        plugin's configured output path using Bank Name.
	 * @param BankName Asset name when creating, e.g. "SB_AirlockScene".
	 * @param Lines What to author. Line Id is generated from the text when left empty.
	 * @param DefaultSpeakerId Applied to lines here that name no speaker of their own.
	 * @return Content path of the bank.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Authoring")
	static FString CreateOrUpdateSpeechBank(
		const FString& AssetPath,
		const FString& BankName,
		const TArray<FSpeechLineSpec>& Lines,
		FName DefaultSpeakerId);

	/**
	 * Create a Speech Voice asset pairing a speaker in this project with a voice on a provider.
	 *
	 * Deliberately does not create anything on the provider. Casting a voice is a human act performed
	 * once, in the provider's own interface, and a tool that could create voices could generally
	 * delete them - which would lose every line that voice ever spoke.
	 *
	 * Get the Provider Voice Id from List Provider Voices. **Record Provenance honestly**: a Premade
	 * voice belongs to the provider and can be retired by it, and knowing which voices are exposed
	 * that way is the difference between a scheduled migration and a library that stops working.
	 *
	 * @param AssetPath Content path to create at. Empty uses the configured voices folder.
	 * @param SpeakerId The project's name for this character. External voice sources match on it.
	 * @param ProviderVoiceId The provider's own id for the voice.
	 * @param ProviderId Leave empty for the default provider.
	 * @param ModelId Leave empty for the project default.
	 * @param Provenance How the voice was made. Guessing here helps nobody.
	 * @return Content path of the voice asset.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Authoring")
	static FString CreateSpeechVoice(
		const FString& AssetPath,
		FName SpeakerId,
		const FString& ProviderVoiceId,
		FName ProviderId,
		const FString& ModelId,
		ESpeechVoiceProvenance Provenance);

	/**
	 * Report which voice a line would actually be spoken in, and why.
	 *
	 * Resolution walks the line's own override, then any registered external source such as an NPC
	 * adapter, then the asset's default, then the project default. Source Description says which of
	 * those answered, which is the only useful thing to read when a line comes out in the wrong
	 * voice.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Authoring")
	static FSpeechVoiceResolution ResolveSpeechVoice(const FSpeechLineHandle& Handle);

	// ---------------------------------------------------------------------------------------------
	// Pipeline
	// ---------------------------------------------------------------------------------------------

	/**
	 * Generate audio for speech lines and wait for it to finish.
	 *
	 * **This spends money, at the moment each request is sent.** Call Estimate Speech Cost first and
	 * get the user's agreement on the number.
	 *
	 * Lines that are already current, already in flight, or whose audio the pipeline no longer owns
	 * are skipped silently. That makes a retry after a timeout safe, and it is also what stops a
	 * regeneration quietly re-performing lines that did not change - which matters more than it
	 * sounds, because these models do not reproduce a take exactly even from the same seed.
	 *
	 * @param Handles Lines to generate. A handle with no Line Id means every line in that asset.
	 * @param Force Regenerate even lines that are already current. Does **not** override graduation -
	 *        a recorded or hand-edited line is never overwritten by this.
	 * @return Per-line status once every request settles.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Pipeline")
	static UToolCallAsyncResultSpeechStatus* GenerateSpeech(
		const TArray<FSpeechLineHandle>& Handles,
		bool Force);

	/**
	 * Fetch a line's existing audio again from the provider, without regenerating it.
	 *
	 * Free where the provider keeps a history, and the right move whenever the sound asset was lost
	 * but the line still knows its request id. Prefer this over regenerating: regenerating costs
	 * money and produces a subtly different performance, where this returns the identical audio.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Pipeline")
	static void RefetchSpeechLine(const FSpeechLineHandle& Handle);

	/**
	 * Accept the audio a line already has, across an authoring change, without regenerating.
	 *
	 * The right answer when a voice was retuned and a whole cast went stale but the existing takes
	 * are fine. Records the line as Accepted rather than Generated, so it stays possible to tell
	 * later which lines actually match what they claim.
	 *
	 * Ask the user before doing this in bulk. It is a judgement about whether a change mattered, and
	 * that is theirs.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Pipeline")
	static void AcceptCurrentSpeechAudio(const FSpeechLineHandle& Handle);

	/**
	 * Point a line at a recorded take - an actor's performance, or any better source.
	 *
	 * The generated audio is kept rather than replaced. It is the reference the performance was
	 * directed against, and a recorded take can be rejected the same afternoon it arrives.
	 *
	 * After this the line is graduated: the pipeline will not overwrite it, and Generate Speech skips
	 * it even with Force set.
	 *
	 * @param RecordedSoundPath Content path of an existing sound asset.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Pipeline")
	static void MarkSpeechLineRecorded(const FSpeechLineHandle& Handle, const FString& RecordedSoundPath);

	/**
	 * Find lines whose audio has been hand-edited since it was generated, and mark them as graduated.
	 *
	 * Worth running before any large regeneration. A generated file that somebody levelled or trimmed
	 * in an audio editor is hand-authored work, and the pipeline overwriting it weeks later is a
	 * failure nobody notices at the time.
	 *
	 * @param Handles Lines to check. Empty means every line in the project.
	 * @return How many lines changed origin.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Pipeline")
	static int32 DetectEditedSpeechAudio(const TArray<FSpeechLineHandle>& Handles);

private:

	/** The subsystem, or a raised script error explaining that the editor is not up. */
	static class USpeechForgeSubsystem* GetSubsystemChecked();

	/**
	 * Poll a batch to completion, then complete the async result with per-line status.
	 *
	 * Line handles are captured up front rather than read back from the batch: a finished batch is
	 * dropped from tracking, so by the time it reports done it no longer knows what it contained.
	 *
	 * Plain C++ rather than a UFUNCTION - the registry publishes every AICallable UFUNCTION on this
	 * class, and a helper is not a tool.
	 */
	static UToolCallAsyncResultSpeechStatus* WatchBatch(
		const FString& BatchId,
		const TArray<FSpeechLineHandle>& Handles);
};
