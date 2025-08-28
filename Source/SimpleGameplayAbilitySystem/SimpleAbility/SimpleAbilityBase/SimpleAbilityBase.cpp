#include "SimpleAbilityBase.h"

#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

bool USimpleAbilityBase::Activate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FGuid NewAbilityID, const FAbilityContextCollection ActivationContext)
{
	// Override this function in child classes but remember to set these variables for easier access in blueprints
	AbilityID = NewAbilityID;
	OwningAbilityComponent = ActivatingAbilityComponent;
	AbilityContexts = ActivationContext;
	ActivationTime = OwningAbilityComponent->GetServerTime();

	return true;
}

void USimpleAbilityBase::OnServerSnapshotReceived(const int32 SnapshotCounter, const FInstancedStruct& AuthoritySnapshotData, const FInstancedStruct& LocalSnapshotData)
{
	if (!PendingSnapshots.Contains(SnapshotCounter))
	{
		return;
	}

	const FOnSnapshotResolved ResolutionFunction = PendingSnapshots.FindAndRemoveChecked(SnapshotCounter);

	if (AuthoritySnapshotData == LocalSnapshotData)
	{
		SIMPLE_LOG(GetWorld(), TEXT("[USimpleAbilityBase::OnServerSnapshotReceived]: Authority and local snapshots are identical. No need to resolve."));
		return;
	}
	
	ResolutionFunction.ExecuteIfBound(AuthoritySnapshotData, LocalSnapshotData);
}

void USimpleAbilityBase::CleanUpAbility()
{
	USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();
	
	if (!EventSubsystem)
	{
		SIMPLE_LOG(this, TEXT("[USimpleAbilityBase::CleanUpAbility]: No SimpleEventSubsystem found."));
		return;
	}

	EventSubsystem->StopListeningForAllEvents(this);
	AbilityContexts = FAbilityContextCollection();
	PendingSnapshots.Empty();
}

FInstancedStruct USimpleAbilityBase::GetAbilityContext(FGameplayTag ContextTag, bool& WasFound) const
{
	for (const FAbilityContext& Context : AbilityContexts.Contexts)
	{
		if (Context.ContextTag.MatchesTagExact(ContextTag))
		{
			WasFound = true;
			return Context.ContextData;
		}
	}

	SIMPLE_LOG(GetWorld(),
		FString::Printf(TEXT("[USimpleAbilityBase::GetAbilityContext]: Context with tag %s not found in AbilityContexts"),
		*ContextTag.ToString()));
	
	WasFound = false;
	return FInstancedStruct();
}

EAbilityNetworkRole USimpleAbilityBase::GetNetworkRole(bool& IsListenServer) const
{
	IsListenServer = OwningAbilityComponent->GetNetMode() == NM_ListenServer;
	
	// Check if we're on a server
	if (OwningAbilityComponent->GetNetMode() < NM_Client)
	{
		return EAbilityNetworkRole::Server;
	}

	return EAbilityNetworkRole::Client;
}

bool USimpleAbilityBase::IsRunningOnClient() const
{
	bool IsListenServer = false;
	return GetNetworkRole(IsListenServer) == EAbilityNetworkRole::Client;
}

bool USimpleAbilityBase::IsRunningOnServer() const
{
	bool IsListenServer = false;
	return GetNetworkRole(IsListenServer) == EAbilityNetworkRole::Server;
}

bool USimpleAbilityBase::HasAuthority() const
{
	return OwningAbilityComponent->HasAuthority();
}

UWorld* USimpleAbilityBase::GetWorld() const
{
	//Return null if called from the CDO, or if the outer is being destroyed
	if (!HasAnyFlags(RF_ClassDefaultObject) &&  !GetOuter()->HasAnyFlags(RF_BeginDestroyed) && !GetOuter()->IsUnreachable())
	{
		//Try to get the world from the owning actor if we have one
		AActor* Outer = GetTypedOuter<AActor>();
		if (Outer != nullptr)
		{
			return Outer->GetWorld();
		}
	}
	
	return nullptr;
}
