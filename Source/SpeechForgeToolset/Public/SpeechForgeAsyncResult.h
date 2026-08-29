// Typed promises for the tools that have to wait on a provider.

#pragma once

#include "CoreMinimal.h"
#include "SpeechForgeTypes.h"
#include "ToolsetRegistry/ToolCallAsyncResult.h"
#include "SpeechForgeAsyncResult.generated.h"

/**
 * An async tool call that completes with per-line status.
 *
 * Generation takes seconds per line and a bank takes minutes, so the tools that drive it return a
 * promise rather than blocking the editor. The registry finds the schema by reflecting over the
 * property literally named Value, which is why this exists rather than reusing the string result: a
 * caller that waited for a batch should get the same typed status the status tool returns, not a
 * string it has to parse. "Finished" only says the requests stopped.
 */
UCLASS(BlueprintType)
class SPEECHFORGETOOLSET_API UToolCallAsyncResultSpeechStatus : public UToolCallAsyncResult
{
	GENERATED_BODY()

public:

	/** Complete the call successfully. Safe to call from any thread. */
	UFUNCTION(BlueprintCallable, Category = "SpeechForge")
	bool SetValue(const TArray<FSpeechLineStatus>& InValue)
	{
		return MaybeBroadcastSuccessfulCompletion(TArray<FSpeechLineStatus>(InValue), Value);
	}

	/** One entry per line the call was watching. */
	UPROPERTY(BlueprintReadOnly, Category = "SpeechForge")
	TArray<FSpeechLineStatus> Value;
};

/** An async tool call that completes with the voices a provider holds. */
UCLASS(BlueprintType)
class SPEECHFORGETOOLSET_API UToolCallAsyncResultSpeechVoices : public UToolCallAsyncResult
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "SpeechForge")
	bool SetValue(const TArray<FSpeechRemoteVoiceInfo>& InValue)
	{
		return MaybeBroadcastSuccessfulCompletion(TArray<FSpeechRemoteVoiceInfo>(InValue), Value);
	}

	UPROPERTY(BlueprintReadOnly, Category = "SpeechForge")
	TArray<FSpeechRemoteVoiceInfo> Value;
};

/** An async tool call that completes with a line of text. */
UCLASS(BlueprintType)
class SPEECHFORGETOOLSET_API UToolCallAsyncResultSpeechString : public UToolCallAsyncResult
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "SpeechForge")
	bool SetValue(const FString& InValue)
	{
		return MaybeBroadcastSuccessfulCompletion(FString(InValue), Value);
	}

	UPROPERTY(BlueprintReadOnly, Category = "SpeechForge")
	FString Value;
};
