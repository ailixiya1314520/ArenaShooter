#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ArenaGameInstance.generated.h"

UCLASS()
class ARENASHOOTER_API UArenaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UArenaGameInstance();

	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "Network")
	void HostLANGame(FString MapName);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void JoinLANGame();

protected:
	FString TargetMapName;

	IOnlineSessionPtr SessionInterface;

	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};