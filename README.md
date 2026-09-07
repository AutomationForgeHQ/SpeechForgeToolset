# SpeechForge Toolset

[SpeechForge](../SpeechForge/README.md) exposed as native Model Context Protocol tools, so an agent
can author speech banks, price them, generate audio and manage graduation without a human driving the
editor.

**Status: 0.1 — 16 tools and one skill, registered and driven live on 2026-08-11.**

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

| | |
|---|---|
| **Discovery** | `ListSpeechAssets`, `GetSpeechLineStatus`, `ListSpeechProviders`, `GetSpeechProviderCapabilities`, `GetSpeechCredentialStatus`, `TestSpeechProviderConnection`, `ListProviderVoices` |
| **Cost** | `EstimateSpeechCost` |
| **Authoring** | `CreateOrUpdateSpeechBank`, `UpdateSpeechLine`, `ResolveSpeechVoice` |
| **Casting** | `CreateVoiceProfile`, `CreateOrUpdateSpeaker`, `ListSpeakers`, `ListVoiceProfiles`, `SetSpeechLineVoiceOverride` |
| **Takes** | `GenerateSpeechTake`, `ListSpeechTakes`, `ApplySpeechTake`, `RegisterRecordedSpeechTake`, `MarkSpeechTakeChosen` |
| **Pipeline** | `GenerateSpeech`, `RefetchSpeechLine`, `AcceptCurrentSpeechAudio`, `MarkSpeechLineRecorded`, `DetectEditedSpeechAudio` |

Casting is two assets and one rule: a **voice profile** is an instrument (provider, preset, model,
settings), a **speaker** is a character sheet that names one, and a line carries only a speaker id.
Resolution walks the line's override, the speaker's sheet, the bank default, the project default.

Takes are the production alternative to latest-wins: `GenerateSpeechTake` makes a candidate without
touching the line, and `ApplySpeechTake` is what makes one the line - so a director can compare
readings instead of regenerating over the last one.

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
