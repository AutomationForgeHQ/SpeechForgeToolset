# SpeechForgeToolset

Every released version of SpeechForgeToolset, newest first. A release publishes **one** section of this
file — the one whose heading matches its tag — as its release notes; for an `open` plugin those
notes are posted to Discord `#releases` automatically. Write for someone who installs the plugin,
not for the commit log.

Headings are `## <x.y.z> — <date>`. Use `Added` / `Changed` / `Fixed` / `Compatibility` /
`Known issues`, only the ones that apply.

## 0.2.2 — 2026-09-11

### Added
- `ProduceSpeechBank` — produce a whole bank the bank's own way, the agent-side twin of the Speech
  Library's Generate All. On a plain bank that generates what is missing or stale; on a localised
  bank it generates the synthesis-sourced lines, dubs the performed ones, and points the face banks
  serving it at the new audio.

### Changed
- The version this toolset reports to an agent is now read from the plugin's own
  descriptor rather than repeated in C++, so it can no longer answer a number the
  installed package does not carry.

## 0.2.1 — 2026-09-08
- Packaging fix: the release now carries everything the register allows. `BuildPlugin`'s filter excludes `Config/` and every `public_extra` path, so earlier zips shipped without them.
- `GetToolsetVersion()` answers this plugin's real version; it had drifted from the descriptor.

## 0.2.0 — 2026-09-07
- Apache-2.0 relicensing; a release now publishes its source
- Every plugin descriptor made to agree with its release tag and credit its author
- The culmination wiring: one library, a teleprompter, and panels that meet
- Speakers, voice profiles, and the three-page Speech Library
- Sessions and takes: recording stops deciding, the director chooses
- Documentation pass to catch up with the code
- `ConvertSpeechLine` shipped — re-voice a line without rewriting it
- Cost estimation extended to the conversion pipeline
- Subtitle-worthiness asked of every line, not just tool-fixable ones
- A re-voicing given its own path, distinct from a new take
- Fixed: pressing Use gave a different voice than expected, and billed for it
- Takes always remembered; "in use" now measured rather than claimed
- Save Takes made a latch; takes can be removed or brought back in
- Speech and face joined by id: a speaker names their head, a bank says what it holds
- Localisation: a sibling bank per language, joined by line id
- Repointed to kovati.dev

## 0.1.0 — 2026-08-28
- Initial release: the pipeline as MCP tools
- Output roots reworked so generated assets move independently of their sources
