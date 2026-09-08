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

	virtual FString GetToolsetVersion() const override { return TEXT("0.2.1"); }

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
	/**
	 * Every speech bank whose source script has changed since the bank last read it.
	 *
	 * **Run this before shipping, and before any pass that trusts subtitles.** A line edited in its
	 * own dialogue editor never reaches the bank that holds its audio, so the script, the subtitle
	 * and the voice drift apart with nothing marked and nobody informed.
	 *
	 * It reports suspicion, not proof: a dialogue resaved without a text change appears here too.
	 * Re-harvesting the named bank is what turns suspicion into an answer - lines whose words
	 * actually moved will then report the drift themselves, and lines that were recorded will say
	 * they need a pickup rather than a regeneration.
	 *
	 * Cheap enough to call freely - it compares file times and only opens banks that have a source.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Status")
	static TArray<FSpeechSourceDrift> CheckSpeechSourceDrift();

	/**
	 * Show a bank - and optionally one line - in the Speech Library.
	 *
	 * Navigation across the pipeline, which nothing had. Every stage knows the asset paths of the
	 * stages around it, so any of them can bring a person to the right place instead of leaving
	 * them to search the content browser for a bank name they half remember.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Navigation")
	static FString OpenSpeechLibrary(const FString& BankPath, const FString& LineId);

	/**
	 * What a bank was harvested from, as "<adapter>|<asset path>". Empty when it is its own source.
	 *
	 * The link back up the pipeline: a bank knows its script, so a panel showing lines can offer to
	 * open the dialogue those lines came from.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Navigation")
	static FString GetSpeechBankSource(const FString& BankPath);

	/**
	 * What generating this bank would cost, without generating anything. Call it before
	 * every run that spends: this is the tool the skill means by "price it first".
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Cost")
	static FSpeechCostEstimate EstimateSpeechCost(
		const TArray<FSpeechLineHandle>& Handles,
		bool IncludeCurrent);

	/**
	 * Price what re-voicing these lines would cost, before anything is sent.
	 *
	 * **Call this before Convert Speech Line.** Conversion is the pipeline's other meter: it bills
	 * by the duration of the audio, not by what the line says, so the same sentence can cost three
	 * times as much read slowly. Estimate Speech Cost cannot answer this - it counts characters.
	 *
	 * It is exact only once the audio exists. Unpriced Count is the honest part of the answer: those
	 * lines would bill but their source could not be measured, and they are left out of the total
	 * rather than guessed at. If everything comes back unpriced, the sources have not been recorded
	 * yet - say so rather than reporting a cost of zero.
	 *
	 * @param Handles Lines to price. A handle with no Line Id means every line in that asset.
	 * @param SourceAudio A sound asset path or an absolute WAV path applied to every line. Leave
	 *        empty to price re-converting each line from whatever it was converted from before.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Cost")
	static FSpeechCostEstimate EstimateSpeechConversionCost(
		const TArray<FSpeechLineHandle>& Handles,
		const FString& SourceAudio);

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
	 * Remove lines from a bank, by id. Ids not present are ignored, so retrying a batch is safe.
	 *
	 * The lines' generated audio assets stay in the project: a bank entry is a reference, and
	 * deleting content is a human's call in the Content Browser.
	 *
	 * @return How many lines were removed.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Authoring")
	static int32 RemoveSpeechLines(const FString& BankPath, const TArray<FName>& LineIds);

	/**
	 * Remove every line from a bank, keeping its defaults, language and source stamp.
	 *
	 * The clean-slate half of changing a bank's source: clear, then harvest the new one. Audio
	 * assets stay, as Remove Speech Lines leaves them.
	 *
	 * @return How many lines were removed.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Authoring")
	static int32 ClearSpeechBank(const FString& BankPath);

	// ---------------------------------------------------------------------------------------------
	// Casting - cast first, write second, produce third
	// ---------------------------------------------------------------------------------------------

	/**
	 * Create or update a voice profile: one voice on one provider, as a reusable project asset.
	 *
	 * The instrument, not the character - name it for the sound ("Warm Male Narrator"), never for a
	 * speaker, and assign it to speakers with Create Or Update Speaker. Two characters can share
	 * one profile, and one project can hold profiles from several providers side by side.
	 *
	 * Deliberately does not create anything on the provider. Making a voice exist is a human act
	 * performed once, in the provider's own interface, and a tool that could create voices could
	 * generally delete them - which would lose every line that voice ever spoke.
	 *
	 * Get Provider Voice Id and the provider's display name from List Provider Voices. **Record
	 * Provenance honestly**: a Premade voice belongs to the provider and can be retired by it.
	 *
	 * @param AssetPath Content path to create at. Empty uses the configured voices folder.
	 * @param DisplayName What this project calls the voice. Panels show this string.
	 * @param ProviderVoiceId The provider's own id for the voice.
	 * @param ProviderVoiceName The provider's display name for it, e.g. "Sarah".
	 * @param ProviderId Leave empty for the default provider.
	 * @param ModelId Leave empty for the project default.
	 * @param Provenance How the voice was made. Guessing here helps nobody.
	 * @return Content path of the profile asset.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Casting")
	static FString CreateVoiceProfile(
		const FString& AssetPath,
		const FString& DisplayName,
		const FString& ProviderVoiceId,
		const FString& ProviderVoiceName,
		FName ProviderId,
		const FString& ModelId,
		ESpeechVoiceProvenance Provenance);

	/**
	 * Create or update a speaker's character sheet - the first step of any scene.
	 *
	 * A dialogue scene has speakers before it has lines: define who is in the scene and what they
	 * sound like here, then write lines that carry the Speaker Id, then generate or record. The
	 * sheet consolidates identity (display name, description), the voice (a profile made with
	 * Create Voice Profile), and links to what the speaker is in other systems.
	 *
	 * Empty Display Name, Description or Voice Profile Path leave the existing values alone, so
	 * seeding identity never un-casts a voice. The sheet is found by Speaker Id wherever it lives.
	 *
	 * @param SpeakerId The id lines carry. Matches the rest of the project's name for the character.
	 * @param VoiceProfilePath Content path of a voice profile, to (re)cast the speaker.
	 * @return Content path of the speaker asset.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Casting")
	static FString CreateOrUpdateSpeaker(
		FName SpeakerId,
		const FString& DisplayName,
		const FString& Description,
		const FString& VoiceProfilePath);

	/** Content paths of every speaker sheet in the project - the cast list. */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Casting")
	static TArray<FString> ListSpeakers();

	/** Content paths of every voice profile in the project. */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Casting")
	static TArray<FString> ListVoiceProfiles();

	/**
	 * Set or clear one line's voice override - the by-hand exception to its speaker's casting.
	 *
	 * Pass an empty Voice Profile Path to clear, returning the line to its speaker's voice. Use
	 * sparingly: the staleness report is per line, and a bank full of overrides is a cast list
	 * nobody can read.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Casting")
	static void SetSpeechLineVoiceOverride(const FSpeechLineHandle& Handle, const FString& VoiceProfilePath);

	/**
	 * Edit a line's authoring fields in place: text, direction, speaker.
	 *
	 * Speaker Id None leaves the speaker alone. Pipeline state is untouched: an edit that changes
	 * what is spoken shows up as stale in Get Speech Line Status, never as a silent regeneration.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Authoring")
	static void UpdateSpeechLine(
		const FSpeechLineHandle& Handle,
		const FString& Text,
		const FString& Direction,
		FName SpeakerId);

	/**
	 * Re-voice a recording into a line's cast voice, and make it the line's audio.
	 *
	 * Speech to speech: the performance keeps its delivery and changes its identity.
	 *
	 * The result inherits the provenance of what it converted. Re-voicing a recording, or any WAV
	 * this pipeline cannot attribute, graduates the line to Recorded so nothing will overwrite a
	 * performance that cannot be regenerated. Re-voicing audio this pipeline synthesised leaves the
	 * line Generated, because it is still machine output and nothing irreplaceable is at stake.
	 *
	 * **This spends money at the moment it is sent**, billed by the duration of the audio rather
	 * than by character, so an eight-second take costs the same whatever it says.
	 *
	 * Afterwards the line is stale only if its **source or its voice** changes - never its text,
	 * which a conversion does not read. Rewriting the script leaves a converted line current, and
	 * correctly so: nothing about the recording changed.
	 *
	 * @param SourceAudio A sound asset's content path, or the absolute path of a WAV on disk.
	 *        Leave empty to re-convert from whatever the line was converted from before, which is
	 *        what a re-cast needs.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Pipeline")
	static UToolCallAsyncResultSpeechString* ConvertSpeechLine(
		const FSpeechLineHandle& Handle,
		const FString& SourceAudio);

	/**
	 * Content path of the sound a line currently plays, or empty. Plumbing for reflection callers;
	 * agents get the same fact, with more around it, from Get Speech Line Status.
	 */
	UFUNCTION()
	static FString GetSpeechLineSoundPath(const FString& AssetPath, const FString& LineId);

	/** Plumbing twin of Convert Speech Line for reflection callers. Agents: use Convert Speech Line. */
	UFUNCTION()
	static UToolCallAsyncResultSpeechString* ConvertSpeechLineByPath(
		const FString& AssetPath,
		const FString& LineId,
		const FString& SourceAudio);

	/**
	 * Report which voice a line would actually be spoken in, and why.
	 *
	 * Resolution walks the line's own override, then the speaker's sheet, then the asset's default,
	 * then the project default. Source Description says which of those answered, which is the only
	 * useful thing to read when a line comes out in the wrong voice.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Authoring")
	static FSpeechVoiceResolution ResolveSpeechVoice(const FSpeechLineHandle& Handle);

	// ---------------------------------------------------------------------------------------------
	// Localisation - a sibling bank per language, joined by line id
	// ---------------------------------------------------------------------------------------------

	/**
	 * The registered translation providers, by id.
	 *
	 * "Pseudo" is always present: the built-in keyless pseudo-localiser, for proving the pipeline
	 * before a vendor key exists. Real providers (e.g. "DeepL") register from their own plugins.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Localisation")
	static TArray<FString> ListSpeechTranslationProviders();

	/**
	 * Translate a bank into a sibling bank for one language, creating SB_<Name>_<LANG> when missing.
	 *
	 * Line ids, speakers, direction and voice overrides carry over untouched; only the text is
	 * translated, and only where it is missing or its source text moved since the last pass (Force
	 * re-translates everything). Audio is deliberately not touched here: generate the localised
	 * bank afterwards exactly like any other - same speakers resolve the same cast voices, the
	 * multilingual models speak whatever language the text is in, and the sounds land in a
	 * per-language folder so they can never overwrite the originals. For the face side, prepare a
	 * face bank from the localised speech bank the ordinary way and solve it from the new audio.
	 *
	 * **Spends translation characters at the moment it is sent** through the named provider - or
	 * the resolved default: the sole real provider, else the built-in "Pseudo".
	 *
	 * @param BankPath Content path of the authoring-language bank.
	 * @param TargetLanguage Language code, e.g. "de" or "pt-BR".
	 * @param TranslationProviderId Which translator. Empty resolves the default.
	 * @param Force Re-translate lines whose translation is already current.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Localisation")
	static UToolCallAsyncResultSpeechString* LocalizeSpeechBank(
		const FString& BankPath,
		const FString& TargetLanguage,
		const FString& TranslationProviderId,
		bool Force);

	/**
	 * Every localised sibling of a bank, counted against today's source text.
	 *
	 * Current means the source line has not been rewritten since it was translated; stale means it
	 * has; missing means it was never translated. Audio staleness is the ordinary per-line status
	 * on the localised bank itself - ask Get Speech Line Status there.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Localisation")
	static TArray<FSpeechLocalizationStatus> GetSpeechLocalizationStatus(const FString& BankPath);

	/**
	 * Dub one localised line from its source recording, keeping the actor's voice and pacing.
	 *
	 * The performance-take path of localisation: where Generate Speech re-reads the translated text
	 * as TTS, a dub carries the original performance - its pauses, its breaths, its delivery - into
	 * the target language, and the timing survives by construction, which is what keeps a captured
	 * face's video layer aligned underneath the new mouth. The line graduates to Recorded.
	 *
	 * The dub's spoken words are the dubbing service's own translation, so the line comes back
	 * words-unverified against the subtitle text - listen once before shipping it.
	 *
	 * **Spends money by the minute of source audio, at a multiple of synthesis rates**, and takes
	 * minutes to render. The handle names a line in a *localised* bank; the source recording is
	 * found through that bank's source link.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Localisation")
	static UToolCallAsyncResultSpeechString* DubSpeechLine(const FSpeechLineHandle& Handle);

	// ---------------------------------------------------------------------------------------------
	// Takes - candidates on a ledger, and a director's choose
	// ---------------------------------------------------------------------------------------------

	/**
	 * Generate one detached candidate for a line: synthesized and imported as its own asset, put on
	 * the line's take ledger, and the line itself untouched. **Spends money like any generation.**
	 * Generate a line several times to compare readings side by side; Apply Speech Take makes the
	 * pick the line. The default flow (Generate Speech) keeps overwriting in place - takes are the
	 * production alternative, not a replacement.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Takes")
	static UToolCallAsyncResultSpeechString* GenerateSpeechTake(const FSpeechLineHandle& Handle);

	/** The line's take ledger: generated candidates and registered recorded performances. */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Takes")
	static TArray<FSpeechLineTake> ListSpeechTakes(const FSpeechLineHandle& Handle);

	/**
	 * Make a take the line. A generated take applies its stored facts exactly as generation would
	 * have; a recorded take goes through graduation. Re-choosing a different take later is just
	 * another apply - hashes keep everything honest.
	 *
	 * @return What was applied, or "ERROR: ..." with the reason.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Takes")
	static FString ApplySpeechTake(const FSpeechLineHandle& Handle, FName TakeId);

	/**
	 * Plumbing for reflection callers - the PerformanceForge ledger reads take rows through this.
	 * One packed string per take: TakeId|Kind|Duration|Created|SoundPath|TakeDir|Chosen. Agents:
	 * use List Speech Takes, which is typed; this exists because reflection callers cannot read a
	 * struct array back and a packed row is honest plumbing rather than a tool schema.
	 */
	UFUNCTION()
	static TArray<FString> ListSpeechTakeRows(const FString& AssetPath, const FString& LineId);

	/** Plumbing twin of Apply Speech Take: plain strings, because struct params do not survive
	 *  reflection import. Agents: use Apply Speech Take. */
	UFUNCTION()
	static FString ApplySpeechTakeByPath(const FString& AssetPath, const FString& LineId, const FString& TakeId);

	/** Plumbing twin of Generate Speech Take, for the same reason. Agents: use Generate Speech Take. */
	UFUNCTION()
	static UToolCallAsyncResultSpeechString* GenerateSpeechTakeByPath(const FString& AssetPath, const FString& LineId);

	/**
	 * Re-voice one take, as another voice for that same performance.
	 *
	 * **Not a new take, and deliberately so.** Speech to speech keeps the delivery, the timing and
	 * the face that were captured once and changes only who it sounds like. Listing that beside the
	 * original as a second candidate would put two rows in the ledger that are the same moment, and
	 * a director choosing between them would be choosing nothing. It lands under the take instead,
	 * as one more voice that take can be heard in.
	 *
	 * Billed by the duration of the take's audio - see Estimate Speech Conversion Cost. The new
	 * voice becomes the take's picked one, because the only reason to make it is to hear it.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Takes")
	static UToolCallAsyncResultSpeechString* ConvertSpeechTake(const FSpeechLineHandle& Handle, FName TakeId);

	/** Path-and-id twin of Convert Speech Take, for callers that cannot pass a struct. */
	UFUNCTION(BlueprintCallable, Category = "SpeechForge|Takes")
	static UToolCallAsyncResultSpeechString* ConvertSpeechTakeByPath(
		const FString& AssetPath,
		const FString& LineId,
		const FString& TakeId);

	/**
	 * Choose which of a take's voices wins if that take is chosen. Empty restores its own recording.
	 *
	 * The picture and the performance are unaffected either way - this decides only what is heard
	 * over them, and what becomes the line's audio if this take is applied.
	 */
	/**
	 * Drop one re-voicing from a take's list. The sound asset stays where it is.
	 *
	 * Tidying, not deleting: an unwanted voice leaves the ledger, and the audio it referred to is
	 * left in the project for whoever wants it.
	 */
	/**
	 * Drop a whole take from a line's ledger.
	 *
	 * Refused while the line is playing that take's audio - choose another take first. Imported
	 * sound assets are left in the project; this clears the ledger, not the content browser.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Takes")
	static FString RemoveSpeechTake(const FString& AssetPath, const FString& LineId, const FString& TakeId);

	/** Removes one variant from a take, leaving the take and its other variants in place. */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Takes")
	static FString RemoveSpeechTakeVariant(
		const FString& AssetPath,
		const FString& LineId,
		const FString& TakeId,
		const FString& VariantId);

	/** Makes one variant of a take the current one. The others are kept, never deleted. */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Takes")
	static FString SetSpeechTakeVariant(
		const FString& AssetPath,
		const FString& LineId,
		const FString& TakeId,
		const FString& VariantId);

	/**
	 * Put a recorded performance on a line's take ledger without applying it. The WAV imports as
	 * its own asset; the capture-take folder is remembered for the video side.
	 *
	 * @return The new take's id.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Takes")
	static FString RegisterRecordedSpeechTake(
		const FString& AssetPath,
		const FString& LineId,
		const FString& WavPath,
		const FString& TakeDir);

	/** Bookkeeping only: record which take is chosen without applying anything. */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Takes")
	static void MarkSpeechTakeChosen(const FString& AssetPath, const FString& LineId, const FString& TakeId);

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
	/**
	 * Make a recorded performance a line's audio. The graduation operation.
	 *
	 * Audio Source is either a sound wave's content path, or the absolute path of a WAV to import.
	 * The line's origin becomes Recorded, the generated take stays linked as the reference, and a
	 * later script edit reads as stale-plus-Recorded - the pickup-session report, never a silent
	 * regeneration. Regeneration will refuse this line from now on; Set Face Clip audio and solve
	 * again for the face side.
	 *
	 * @param AssetPath Content path of the bank or line def holding the line.
	 * @param LineId Which line.
	 * @param AudioSource Sound wave content path, or absolute WAV file path.
	 * @return The applied sound's content path, or "ERROR: ..." with the reason.
	 */
	UFUNCTION(meta = (AICallable), Category = "SpeechForge|Graduation")
	static FString ApplyRecordedAudio(const FString& AssetPath, FName LineId, const FString& AudioSource);

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
