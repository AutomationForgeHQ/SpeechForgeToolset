// What an agent needs to know about SpeechForge that the tool signatures cannot say.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/AgentSkill.h"
#include "SpeechForgeSkill.generated.h"

/**
 * How to drive the SpeechForge pipeline without wasting the user's money or their performances.
 *
 * Deliberately short and free of tool and property names, which rot. What belongs here is the
 * economics and the ordering - the things no signature can express and no model can guess.
 */
UCLASS()
class SPEECHFORGETOOLSET_API USpeechForgeSkill : public UAgentSkill
{
	GENERATED_BODY()

public:

	USpeechForgeSkill()
	{
		Description = TEXT(
			"Generate spoken dialogue from written lines through SpeechForge: authoring speech banks, "
			"pricing them, generating audio with character-level timings, and managing the handover "
			"from generated takes to recorded ones.");

		Instructions = TEXT(
			"SpeechForge turns written lines into imported sound assets with timing data. Five things "
			"shape how you should use it, and the first two will cost the user real money if you get "
			"them wrong.\n"
			"\n"
			"First, the money is spent at the moment a request is sent, and it is spent per character "
			"of input. A failed generation is still a paid one and nothing refunds a bad take. The "
			"estimate is exact rather than a projection, so there is no excuse for not asking: price "
			"the batch, tell the user the number, get their agreement. It is usually small - a line is "
			"about a cent - which is precisely why it is easy to spend a lot without noticing.\n"
			"\n"
			"Second, and less obvious: **regenerating is not free even when it is cheap, because these "
			"models do not reproduce a take.** The same line, the same voice and the same seed come "
			"back subtly different. So regenerating a whole bank because one line changed will quietly "
			"re-perform every other line in it, and a conversation whose lines were recorded at "
			"different moments sounds wrong in a way nobody can point at. Generate only what is stale. "
			"Never pass the force flag to 'fix' something without saying why you are doing it.\n"
			"\n"
			"Third, the script and the direction are different fields and must stay that way. The text "
			"is what the player reads on screen as well as what is spoken. Performance notes go in the "
			"direction field. Put them in the text and the subtitle reads the stage direction out - "
			"the audio will be perfect, the asset will be correct, the log will be clean, and only "
			"somebody looking at the screen will ever find it.\n"
			"\n"
			"Fourth, lines in a conversation should say which line they follow. Providers that support "
			"it are then given the preceding line, and prosody carries across the exchange. It costs "
			"nothing and it is most of the difference between a conversation and a list of sentences.\n"
			"\n"
			"Fifth, some audio is not yours to overwrite. A line whose audio was recorded by an actor, "
			"or hand-edited by somebody in an audio editor, has graduated: the pipeline leaves it "
			"alone, and so should you. The status that matters most is a line that is both stale and "
			"recorded - that means the script was rewritten after it was performed, so it needs a "
			"pickup session and not a regeneration. Surface those to the user as a list. It is the one "
			"report with a real cost attached and nothing else produces it.\n"
			"\n"
			"The normal order is: check the provider has a key, check what voices the account holds, "
			"make sure a voice is paired, author the lines, price the batch, agree the number, "
			"generate, then read the status back. Retrying is safe at every step - lines already in "
			"flight or already current are skipped rather than resubmitted, so a call that timed out "
			"can simply be made again without paying twice.\n"
			"\n"
			"Two things you cannot do, both on purpose. You cannot set an API key; signing in is the "
			"user's job in Project Settings. And you cannot create or delete a voice on the provider; "
			"casting is a human act, and a tool that could create voices could generally delete them, "
			"which would lose every line that voice ever spoke.\n"
			"\n"
			"One thing worth telling the user unprompted: if their voices are the provider's stock "
			"ones, those can be retired by the provider and would take the whole library with them. "
			"Designing or cloning a voice of their own is a five-minute job now and a re-record later.");
	}
};
