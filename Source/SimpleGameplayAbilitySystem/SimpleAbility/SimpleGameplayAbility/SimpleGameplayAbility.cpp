#include "SimpleGameplayAbility.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/SimpleAttributeComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleSubAbility/SimpleSubAbility.h"

void USimpleGameplayAbility::Initialize(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, USimpleAttributeComponent* ActivatingAttributeComponent, const FGuid NewAbilityID)
{
	AbilityID = NewAbilityID;
	AbilityComponent = ActivatingAbilityComponent;
	// Cache a reference to the attribute component
	AttributeComponent = ActivatingAttributeComponent;
	ActivationTime = AbilityComponent->GetServerTime();
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
		if (!AbilityContext.GetScriptStruct())
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Ability %s requires a context type of %s but no context was passed in."), *GetName(), *RequiredContextType->GetName()));
			return false;
		}
		
		if (AbilityContext.GetScriptStruct() != RequiredContextType)
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Ability %s requires a context type of %s but context of type %s was passed in."), *GetName(), *RequiredContextType->GetName(), *AbilityContext.GetScriptStruct()->GetName()));
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
	
	return CanActivate(AbilityContext);
}

USimpleSubAbility* USimpleGameplayAbility::ActivateSubAbility(TSubclassOf<USimpleSubAbility> AbilityClass, FInstancedStruct ActivationContext, EAbilityActivationResult& ActivationResult)
{
	if (!AbilityClass)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::ActivateSubAbility]: AbilityClass is null for parent ability %s"), *GetName()));
		return nullptr;
	}

	// Create a new instance of the sub-ability
	USimpleSubAbility* SubAbilityInstance = GetSubAbilityInstance(AbilityClass);

	if (!SubAbilityInstance)
	{
		ActivationResult = EAbilityActivationResult::ActivationFailed;
		return nullptr;
	}

	// Bind to the sub-ability's lifecycle events before activation to handle same-frame end/cancel
	SubAbilityInstance->OnAbilityEnded.AddDynamic(this, &USimpleGameplayAbility::OnSubAbilityEnded);
	SubAbilityInstance->OnAbilityCancelled.AddDynamic(this, &USimpleGameplayAbility::OnSubAbilityCancelled);

	// Activate the sub-ability
	const bool WasActivated = SubAbilityInstance->ActivateAbility(ActivationContext);

	if (!WasActivated)
	{
		SubAbilityInstance->OnAbilityEnded.RemoveDynamic(this, &USimpleGameplayAbility::OnSubAbilityEnded);
		SubAbilityInstance->OnAbilityCancelled.RemoveDynamic(this, &USimpleGameplayAbility::OnSubAbilityCancelled);

		SubAbilityInstances.RemoveAll([SubAbilityInstance](const FActivatedSubAbility& Item)
		{
			return Item.SubAbilityInstance == SubAbilityInstance;
		});

		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::ActivateSubAbility]: Failed to activate sub-ability of class %s"), *AbilityClass->GetName()));
		ActivationResult = EAbilityActivationResult::ActivationFailed;
		return nullptr;
	}

	ActivationResult = EAbilityActivationResult::Activated;
	return SubAbilityInstance;
}

USimpleSubAbility* USimpleGameplayAbility::GetSubAbilityInstance(const TSubclassOf<USimpleSubAbility> AbilityClass)
{
	// Create a new instance of the sub-ability
	USimpleSubAbility* SubAbilityInstance = NewObject<USimpleSubAbility>(this, AbilityClass);
	if (!SubAbilityInstance)
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::ActivateSubAbility]: Failed to create sub-ability instance of class %s"), *AbilityClass->GetName()));
		return nullptr;
	}

	// Initialize the sub-ability with reference to this parent ability
	SubAbilityInstance->Initialize(this, AbilityID);

	// Set up lifecycle listeners to track when the sub-ability completes
	FActivatedSubAbility ActivatedSubAbility;
	ActivatedSubAbility.AbilityID = FGuid::NewGuid();
	ActivatedSubAbility.SubAbilityInstance = SubAbilityInstance;

	// Store the sub-ability info before activation
	SubAbilityInstances.Add(ActivatedSubAbility);

	return SubAbilityInstance;
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

	// Cancel/End sub-abilities based on their cancellation policy
	TArray<ESubAbilityCancellationPolicy> CancelPolicies;
	CancelPolicies.Add(ESubAbilityCancellationPolicy::CancelOnParentAbilityEndedOrCancelled);
	if (WasCancelled)
	{
		CancelPolicies.Add(ESubAbilityCancellationPolicy::CancelOnParentAbilityCancelled);
	}
	else
	{
		CancelPolicies.Add(ESubAbilityCancellationPolicy::CancelOnParentAbilityEnded);
	}

	// Iterate backwards to safely handle removal during iteration
	// (CancelAbility triggers OnAbilityCancelled which calls RemoveTrackedSubAbilityByInstance)
	for (int32 i = SubAbilityInstances.Num() - 1; i >= 0; --i)
	{
		const FActivatedSubAbility& SubAbility = SubAbilityInstances[i];

		if (!SubAbility.SubAbilityInstance)
		{
			continue;
		}

		if (!SubAbility.SubAbilityInstance->IsActive)
		{
			continue;
		}

		// Check if we should cancel this sub-ability based on its policy
		if (!CancelPolicies.Contains(SubAbility.SubAbilityInstance->CancellationPolicy))
		{
			continue;
		}

		// Cancel the sub-ability
		SubAbility.SubAbilityInstance->CancelAbility(FDefaultTags::SubAbilityCancelled(), EndingContext);
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

EAbilityNetworkRole USimpleGameplayAbility::GetNetworkRole() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return EAbilityNetworkRole::Client;
	}

	switch (World->GetNetMode())
	{
		case NM_DedicatedServer:
			return EAbilityNetworkRole::DedicatedServer;
		case NM_ListenServer:
			return EAbilityNetworkRole::ListenServer;
		case NM_Client:
			return EAbilityNetworkRole::Client;
		case NM_Standalone:
		default:
			// Treat standalone as ListenServer for network role purposes
			return EAbilityNetworkRole::ListenServer;
	}
}

bool USimpleGameplayAbility::HasAuthority() const
{
	return GetWorld()->GetNetMode() < NM_Client;
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

	// No need to resolve snapshots if they are identical
	if (AuthoritySnapshotData == LocalSnapshotData)
	{
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

// --- Sub-ability delegate handlers ---

void USimpleGameplayAbility::OnSubAbilityEnded(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext)
{
	RemoveTrackedSubAbilityByInstance(AbilityInstance);
}

void USimpleGameplayAbility::OnSubAbilityCancelled(USimpleAbilityBase* AbilityInstance, FGameplayTag StopStatus, FInstancedStruct StopContext)
{
	RemoveTrackedSubAbilityByInstance(AbilityInstance);
}

void USimpleGameplayAbility::RemoveTrackedSubAbilityByInstance(USimpleAbilityBase* AbilityInstance)
{
	if (!AbilityInstance)
	{
		return;
	}

	AbilityInstance->OnAbilityEnded.RemoveDynamic(this, &USimpleGameplayAbility::OnSubAbilityEnded);
	AbilityInstance->OnAbilityCancelled.RemoveDynamic(this, &USimpleGameplayAbility::OnSubAbilityCancelled);

	SubAbilityInstances.RemoveAll([AbilityInstance](const FActivatedSubAbility& Item)
	{
		return Item.SubAbilityInstance == AbilityInstance;
	});
}
