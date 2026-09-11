# SpeechForge Toolset

[SpeechForge](https://github.com/AutomationForgeHQ/SpeechForge) exposed as native Model Context Protocol tools, so an agent
can author speech banks, price them, generate audio and manage graduation without a human driving the
editor.

**Status: 0.2 — <!-- forge:tools -->45 tools<!-- /forge:tools --> and one skill.** First registered and driven live on 2026-08-11 with a
smaller surface; grown since as localisation and take-level tools shipped.

Adapter only. Every function forwards to `USpeechForgeSubsystem` and adds nothing; all logic, state
and safety live in the capability plugin. **Delete this plugin and SpeechForge behaves identically.**

---

## Why it is a separate plugin

`ToolsetRegistry` and `ModelContextProtocol` are Experimental. Folding these tools into SpeechForge
would make the capability plugin refuse to load anywhere those are switched off — which is most
places, since both are off by default.

That is the same capability/adapter split the plugin family holds everywhere, applied to a surface
rather than a framework.

---

## The tools

<!-- forge:tools -->45 tools<!-- /forge:tools --> across eleven categories. Every row is grounded in the function's own doc comment.

### Discovery

| Tool | What it does |
|---|---|
| `ListSpeechAssets` | List the content paths of speech banks and single-line speech assets in the project. |
| `GetSpeechLineStatus` | Report status, origin, staleness and resolved voice for speech lines. |
| `ListSpeechProviders` | List the speech providers this project has compiled in. |
| `GetSpeechProviderCapabilities` | Report what a provider can actually do: sample rate, billing, seeding, timings, concurrency. |
| `GetSpeechCredentialStatus` | Report whether a provider has a usable API key, and where it is read from. |
| `TestSpeechProviderConnection` | Make one cheap authenticated call to a provider and report what came back. |
| `ListProviderVoices` | List the voices this account holds on a provider. |

### Output

| Tool | What it does |
|---|---|
| `GetSpeechOutputPaths` | Where the pipeline writes: the configured root, and every folder derived from it. |
| `SetSpeechOutputRoot` | Point the pipeline at a different content root. Banks, Voices and Sounds beneath it stay derived; nothing already written moves. |

### Status

| Tool | What it does |
|---|---|
| `CheckSpeechSourceDrift` | Every speech bank whose source script has changed since the bank last read it. |

### Navigation

| Tool | What it does |
|---|---|
| `OpenSpeechLibrary` | Show a bank - and optionally one line - in the Speech Library. |
| `GetSpeechBankSource` | What a bank was harvested from, as `"<adapter>|<asset path>"`. Empty when it is its own source. |

### Cost

| Tool | What it does |
|---|---|
| `EstimateSpeechCost` | Price what generating these lines would cost, before anything is sent. Exact, per character, no network call. |
| `EstimateSpeechConversionCost` | Price what re-voicing these lines would cost, before anything is sent. Billed by audio duration, not characters. |

### Authoring

| Tool | What it does |
|---|---|
| `CreateOrUpdateSpeechBank` | Create a speech bank, or add lines to one that already exists. |
| `RemoveSpeechLines` | Remove lines from a bank, by id. Generated audio assets stay in the project. |
| `ClearSpeechBank` | Remove every line from a bank, keeping its defaults, language and source stamp. |
| `UpdateSpeechLine` | Edit a line's authoring fields in place: text, direction, speaker. |
| `ResolveSpeechVoice` | Report which voice a line would actually be spoken in, and why. |

### Casting

A **voice profile** is an instrument (provider, preset, model, settings); a **speaker** is a
character sheet that names one; a line carries only a speaker id. Resolution walks the line's
override, the speaker's sheet, the bank default, the project default.

| Tool | What it does |
|---|---|
| `CreateVoiceProfile` | Create or update a voice profile: one voice on one provider, as a reusable project asset. |
| `CreateOrUpdateSpeaker` | Create or update a speaker's character sheet - the first step of any scene. |
| `ListSpeakers` | Content paths of every speaker sheet in the project - the cast list. |
| `ListVoiceProfiles` | Content paths of every voice profile in the project. |
| `SetSpeechLineVoiceOverride` | Set or clear one line's voice override - the by-hand exception to its speaker's casting. |

### Localisation

A sibling bank per language, joined by line id.

| Tool | What it does |
|---|---|
| `ListSpeechTranslationProviders` | The registered translation providers, by id. `"Pseudo"`, the built-in keyless pseudo-localiser, is always present. |
| `LocalizeSpeechBank` | Translate a bank into a sibling bank for one language, creating `SB_<Name>_<LANG>` when missing. Spends translation characters at submission. |
| `GetSpeechLocalizationStatus` | Every localised sibling of a bank, counted against today's source text: current, stale or missing. |
| `DubSpeechLine` | Dub one localised line from its source recording, keeping the actor's voice and pacing. Graduates the line to Recorded. |

### Takes

The production alternative to latest-wins: `GenerateSpeechTake` makes a candidate without touching
the line, and `ApplySpeechTake` is what makes one the line - so a director can compare readings
instead of regenerating over the last one.

| Tool | What it does |
|---|---|
| `GenerateSpeechTake` | Generate one detached candidate for a line, put on its take ledger, line itself untouched. Spends money like any generation. |
| `ListSpeechTakes` | The line's take ledger: generated candidates and registered recorded performances. |
| `ApplySpeechTake` | Make a take the line. A generated take applies its stored facts; a recorded take goes through graduation. |
| `ConvertSpeechTake` | Re-voice one take, as another voice for that same performance - not a new take, deliberately. |
| `RemoveSpeechTake` | Drop a whole take from a line's ledger. Refused while the line is playing that take's audio. |
| `RemoveSpeechTakeVariant` | Drop one re-voicing from a take's list. The sound asset it referred to stays in the project. |
| `SetSpeechTakeVariant` | Choose which of a take's voices wins if that take is chosen. Empty restores its own recording. |
| `RegisterRecordedSpeechTake` | Put a recorded performance on a line's take ledger without applying it. |
| `MarkSpeechTakeChosen` | Bookkeeping only: record which take is chosen without applying anything. |

### Pipeline

| Tool | What it does |
|---|---|
| `ConvertSpeechLine` | Re-voice a recording into a line's cast voice, and make it the line's audio - speech to speech. Spends money at the moment it is sent. |
| `GenerateSpeech` | Generate audio for speech lines and wait for it to finish. Spends money at the moment each request is sent. |
| `RefetchSpeechLine` | Fetch a line's existing audio again from the provider, without regenerating it. Free where the provider keeps a history. |
| `AcceptCurrentSpeechAudio` | Accept the audio a line already has, across an authoring change, without regenerating. |
| `MarkSpeechLineRecorded` | Point a line at a recorded take - an actor's performance, or any better source. Graduates the line. |
| `DetectEditedSpeechAudio` | Find lines whose audio has been hand-edited since it was generated, and mark them as graduated. |

### Graduation

| Tool | What it does |
|---|---|
| `ApplyRecordedAudio` | Make a recorded performance a line's audio. The graduation operation underneath Mark Speech Line Recorded. |

---

## Conventions this holds to

**The signature is the schema.** Parameters and returns are `USTRUCT`s and enums, never JSON strings.
A tool typed `FString Json` publishes a schema that says "a string" and leaves the model guessing
field names out of prose; a tool typed `TArray<FSpeechLineSpec>` publishes every field, its type, its
clamp and its tooltip. This is why the *subsystem* underneath is typed rather than JSON-first.

**Doc comments are user-facing copy.** They become the tool descriptions an agent reads before
calling anything, so they are written for that reader — the economics, the ordering, the thing that
will cost money — not for us.

**Errors are raised, not returned.** `RaiseScriptError`; the registry turns script exceptions into
tool errors. No `{"ok": false}` envelope to check on every call, and every message names what to do
next.

**Async tools return typed promises.** `UToolCallAsyncResultSpeechStatus` completes with per-line
status rather than a batch summary — "finished" only says the requests stopped, and what a caller
actually needs is which lines came back stale, failed, or graduated.

**Registration is one explicit line.** Native `UAgentSkill` classes are auto-discovered; toolsets are
**not**. `FSpeechForgeToolsetModule::StartupModule` calls `UToolsetRegistry::RegisterToolsetClass`,
and forgetting it makes the entire toolset invisible with no warning anywhere — which reads exactly
like the plugin being disabled.

---

## Two deliberate omissions

**No tool sets an API key.** An agent that can write secrets into the OS credential vault is a
liability with no matching benefit. Agents can ask whether a key exists, never set or read one.

**No tool creates or deletes a voice on the provider.** A key that can create a voice can generally
delete one, and deleting a voice loses every line it ever spoke. Casting is a human act, done once,
in the provider's own interface.

Both are the same principle: the operations where an agent's speed converts a small mistake into an
irreversible one do not get an automated surface.

---

## The skill

`USpeechForgeSkill` ships beside the toolset and carries what no signature can express. It is short
and deliberately free of tool and property names, which rot.

What it says, because a model will otherwise get it wrong:

- Money is spent **at submission**, per character. A failed generation is still a paid one.
- **Regenerating is not free even when it is cheap**, because these models do not reproduce a take —
  so regenerating a bank quietly re-performs every line in it.
- Direction belongs in its own field. In the text, it ends up in the subtitle.
- Lines in a conversation should name their predecessor.
- Recorded and hand-edited audio is not yours to overwrite, and **stale plus recorded** is the one
  report with real money attached.

Verified discoverable at `/Script/SpeechForgeToolset.SpeechForgeSkill`.

---

## Driving it

Nothing here needs this repo's tooling — any MCP client works. With the project's own helper:

```powershell
$ue = "C:\Users\night\.claude\skills\unreal-mcp\ue-mcp.ps1"
$t  = "SpeechForgeToolset.SpeechForgeToolset"

& $ue call $t TestSpeechProviderConnection '{"ProviderId":"ElevenLabs"}'
& $ue call $t EstimateSpeechCost '{"Handles":[{"AssetPath":"/Game/.../SB_AirlockScene.SB_AirlockScene","LineId":""}],"IncludeCurrent":false}'
& $ue call $t GenerateSpeech    '{"Handles":[{"AssetPath":"/Game/.../SB_AirlockScene.SB_AirlockScene","LineId":""}],"Force":false}'
```

A handle with an empty `LineId` means **every line in that asset**. An empty `Handles` array means
every line in the project — cheap for status and pricing, and worth thinking about before generating.
