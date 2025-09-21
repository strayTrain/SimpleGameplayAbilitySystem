#include "SimpleGameplayAbility.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

USimpleAttributeComponent* USimpleGameplayAbility::GetAttributeComponent_Implementation()
{
	// By default, we assume the attribute component is on the same actor as the ability component
	return AbilityComponent->GetOwner()->GetComponentByClass<USimpleAttributeComponent>();
}

void USimpleGameplayAbility::OnGrantedStatic(TSubclassOf<USimpleGameplayAbility> AbilityClass, USimpleGameplayAbilityComponent* GrantedAbilityComponent)
{
	// CDO = Class Default Object
	USimpleGameplayAbility* CDO = Cast<USimpleGameplayAbility>(AbilityClass->GetDefaultObject());
    
	if (CDO)
	{
		CDO->OnGranted(GrantedAbilityComponent);
	}
}

bool USimpleGameplayAbility::CanActivateInternal()
{
	// Check if the ability is granted
	if (RequireGrantToActivate && !AbilityComponent->GrantedAbilities.Contains(GetClass()))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Ability %s is not granted!"), *GetName()));
		return false;
	}
	
	// Check required tags
	if (!AttributeComponent->HasAllGameplayTags(ActivationRequiredTags))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Required tags for %s not present on ability component"), *GetName()));
		return false;
	}

	if (AttributeComponent->HasAnyGameplayTags(ActivationBlockingTags))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: A blocking tags for %s is present on ability component"), *GetName()));
		return false;
	}

	// Check required context types
	if (RequiredContextType)
	{
		if (!Context.GetScriptStruct())
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Ability %s requires a context type of %s but no context was passed in."), *GetName(), *RequiredContextType->GetName()));
			return false;
		}
		
		if (Context.GetScriptStruct() != RequiredContextType)
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Ability %s requires a context type of %s but context of type %s was passed in."), *GetName(), *RequiredContextType->GetName(), *Context.GetScriptStruct()->GetName()));
			return false;
		}
	}

	// Check avatar actor type
	if (AvatarTypeFilter.Num() > 0)
	{
		const AActor* AvatarActor = GetAvatarActor();

		if (!AvatarActor)
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Avatar actor is null when trying to activate %s"), *GetName()));
			return false;
		}

		if (!AvatarTypeFilter.Contains(AvatarActor->GetClass()))
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Avatar actor %s is not of a valid type when trying to activate %s"), *AvatarActor->GetName(), *GetName()));
			return false;
		}
	}
	
	return CanActivate(Context);
}

void USimpleGameplayAbility::Initialize(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FGuid NewAbilityID)
{
	AbilityID = NewAbilityID;
	AbilityComponent = ActivatingAbilityComponent;
	// Cache a reference to the attribute component
	AttributeComponent = GetAttributeComponent();
	ActivationTime = AbilityComponent->GetServerTime();
}

void USimpleGameplayAbility::PreActivateInternal()
{
	for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
	{
		AttributeComponent->AddGameplayTag(TempTag);
	}

	for (const FGameplayTag& PermTag : PermanentlyAppliedTags)
	{
		AttributeComponent->AddGameplayTag(PermTag);
	}
}

void USimpleGameplayAbility::AbilityEndedInternal(FInstancedStruct EndingContext, bool WasCancelled)
{
	for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
	{
		AttributeComponent->RemoveGameplayTag(TempTag);
	}

	TArray<ESubAbilityCancellationPolicy> CancelPolicies;
	CancelPolicies.Add(ESubAbilityCancellationPolicy::CancelOnParentAbilityEndedOrCancelled);
	CancelPolicies.Add(ESubAbilityCancellationPolicy::CancelOnParentAbilityCancelled);

	TArray<ESubAbilityCancellationPolicy> EndPolicies;
	EndPolicies.Add(ESubAbilityCancellationPolicy::CancelOnParentAbilityEndedOrCancelled);
	EndPolicies.Add(ESubAbilityCancellationPolicy::CancelOnParentAbilityEnded);
	
	for (const FActivatedSubAbility SubAbility : ActivatedSubAbilities)
	{
		if (WasCancelled && !CancelPolicies.Contains(SubAbility.CancellationPolicy))
		{
			continue;
		}

		if (!EndPolicies.Contains(SubAbility.CancellationPolicy))
		{
			continue;
		}
		
		AbilityComponent->CancelAbility(SubAbility.AbilityID, EndingContext);
	}
}

void USimpleGameplayAbility::TakeStateSnapshot(const FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved)
{
	if (!AbilityComponent)
	{
		SIMPLE_LOG(GetWorld(), FString::Printf(TEXT("[USimpleAbilityBase::TakeSnapshot]: AbilityComponent is null. Cannot take snapshot.")));
		return;
	}

	const int32 SnapshotCounter = AbilityComponent->AddGameplayAbilitySnapshot(AbilityID, SnapshotData);
	PendingSnapshots.Add(SnapshotCounter, OnResolved);
}

EAbilityNetworkRole USimpleGameplayAbility::GetNetworkRole(bool& IsListenServer) const
{
	IsListenServer = GetWorld()->GetNetMode() == NM_ListenServer;
	
	// Check if we're on a server
	if (GetWorld()->GetNetMode() < NM_Client)
	{
		return EAbilityNetworkRole::Server;
	}

	return EAbilityNetworkRole::Client;
}

bool USimpleGameplayAbility::IsRunningOnClient() const
{
	bool IsListenServer = false;
	return GetNetworkRole(IsListenServer) == EAbilityNetworkRole::Client;
}

bool USimpleGameplayAbility::IsRunningOnServer() const
{
	bool IsListenServer = false;
	return GetNetworkRole(IsListenServer) == EAbilityNetworkRole::Server;
}

bool USimpleGameplayAbility::HasAuthority() const
{
	return GetWorld()->GetNetMode() < NM_Client;
}

FGuid USimpleGameplayAbility::ActivateSubAbility(
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	FInstancedStruct ActivationContext,
	ESubAbilityCancellationPolicy CancellationPolicy)
{
	const FGuid SubAbilityID = FGuid::NewGuid();

	FActivatedSubAbility SubAbility;
	SubAbility.AbilityID = SubAbilityID;
	SubAbility.CancellationPolicy = CancellationPolicy;
	ActivatedSubAbilities.Add(SubAbility);

	//AbilityComponent->ActivateAbilityWithID(SubAbilityID, AbilityClass, ActivationContext);

	return SubAbilityID;
}

AActor* USimpleGameplayAbility::GetAvatarActor() const
{
	if (!AbilityComponent)
	{
		SIMPLE_LOG(AbilityComponent,FString::Printf(TEXT("Ability component is null for ability %s"), *GetName()));
		return nullptr;
	}

	if (!AbilityComponent->GetAvatarActor())
	{
		SIMPLE_LOG(AbilityComponent,FString::Printf(TEXT("Owning ability component has no avatar actor for ability %s"), *GetName()));
		return nullptr;
	}

	return AbilityComponent->GetAvatarActor();
}

AActor* USimpleGameplayAbility::GetAvatarActorAs(TSubclassOf<AActor> AvatarClass, bool& IsValid) const
{
	AActor* AvatarActor = GetAvatarActor();

	if (!AvatarActor)
	{
		IsValid = false;
		return nullptr;
	}

	if (!AvatarActor->IsA(AvatarClass))
	{
		SIMPLE_LOG(AbilityComponent,FString::Printf(TEXT("Avatar actor %s is not of type %s"), *AvatarActor->GetName(), *AvatarClass->GetName()));
		IsValid = false;
		return AvatarActor;
	}
	
	IsValid = true;
	return AvatarActor;
}

void USimpleGameplayAbility::OnServerSnapshotReceived(const int32 SnapshotCounter, const FInstancedStruct& AuthoritySnapshotData, const FInstancedStruct& LocalSnapshotData)
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

double USimpleGameplayAbility::GetActivationTime() const
{
	return ActivationTime;
}

double USimpleGameplayAbility::GetActivationDelay() const
{
	return AbilityComponent->GetServerTime() - GetActivationTime();
}
