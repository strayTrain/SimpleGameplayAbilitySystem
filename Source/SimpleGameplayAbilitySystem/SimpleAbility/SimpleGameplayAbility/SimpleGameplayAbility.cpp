#include "SimpleGameplayAbility.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

void USimpleGameplayAbility::OnGrantedStatic(TSubclassOf<USimpleGameplayAbility> AbilityClass, USimpleGameplayAbilityComponent* GrantedAbilityComponent)
{
	// CDO = Class Default Object
	USimpleGameplayAbility* CDO = Cast<USimpleGameplayAbility>(AbilityClass->GetDefaultObject());
    
	if (CDO)
	{
		CDO->OnGranted(GrantedAbilityComponent);
	}
}

bool USimpleGameplayAbility::CanActivate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FAbilityContextCollection ActivationContext)
{
	// Check if the ability is granted
	if (RequireGrantToActivate && !ActivatingAbilityComponent->GrantedAbilities.Contains(GetClass()))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Ability %s is not granted!"), *GetName()));
		return false;
	}
	
	// Check required tags
	if (!ActivatingAbilityComponent->HasAllGameplayTags(ActivationRequiredTags))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: Required tags for %s not present on ability component"), *GetName()));
		return false;
	}

	if (ActivatingAbilityComponent->HasAnyGameplayTags(ActivationBlockingTags))
	{
		SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: A blocking tags for %s is present on ability component"), *GetName()));
		return false;
	}

	// Check required context types
	if (RequiredContextTypes.Num() > 0)
	{
		for (const FAbilityContext Context : ActivationContext.Contexts)
		{
			if (!RequiredContextTypes.Contains(Context.ContextData.GetScriptStruct()))
			{
				SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::CanActivate]: A required context type is not present when trying to activate %s"), *GetName()));
				return false;
			}
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
	
	return CanAbilityActivate(ActivatingAbilityComponent, ActivationContext);
}

bool USimpleGameplayAbility::Activate(USimpleGameplayAbilityComponent* ActivatingAbilityComponent, const FGuid NewAbilityID, const FAbilityContextCollection ActivationContext)
{
	if (!CanActivate(ActivatingAbilityComponent, ActivationContext))
	{
		return false;
	}

	AbilityID = NewAbilityID;
	OwningAbilityComponent = ActivatingAbilityComponent;
	AbilityContexts = ActivationContext;
	ActivationTime = OwningAbilityComponent->GetServerTime();
	IsActive = true;
	
	OnActivationSuccess.ExecuteIfBound(AbilityID);
	
	for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
	{
		OwningAbilityComponent->AddGameplayTag(TempTag, FInstancedStruct());
	}

	for (const FGameplayTag& PermTag : PermanentlyAppliedTags)
	{
		OwningAbilityComponent->AddGameplayTag(PermTag, FInstancedStruct());
	}
	
	OnPreActivate();
	OnActivate();
	
	return true;
}

void USimpleGameplayAbility::Cancel(FGameplayTag CancelStatus, FInstancedStruct CancelContext)
{
	IsActive = false;
	OnAbilityCancelled.ExecuteIfBound(AbilityID, CancelStatus, CancelContext);
	OnCancel(CancelStatus, CancelContext);
	OnAbilityStopped(CancelContext, true);
}

void USimpleGameplayAbility::End(FGameplayTag EndStatus, FInstancedStruct EndContext)
{
	IsActive = false;
	OnAbilityEnded.ExecuteIfBound(AbilityID, EndStatus, EndContext);
	OnEnd(EndStatus, EndContext);
	OnAbilityStopped(EndContext, false);
}

void USimpleGameplayAbility::TakeSnapshotInternal(const FInstancedStruct SnapshotData, const FOnSnapshotResolved& OnResolved)
{
	if (!OwningAbilityComponent)
	{
		SIMPLE_LOG(GetWorld(), FString::Printf(TEXT("[USimpleAbilityBase::TakeSnapshot]: OwningAbilityComponent is null. Cannot take snapshot.")));
		return;
	}

	const int32 SnapshotCounter = OwningAbilityComponent->AddGameplayAbilitySnapshot(AbilityID, SnapshotData);
	PendingSnapshots.Add(SnapshotCounter, OnResolved);
}

void USimpleGameplayAbility::OnAbilityStopped(FInstancedStruct& StopContext, bool WasCancelled)
{
	for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
	{
		OwningAbilityComponent->RemoveGameplayTag(TempTag, FInstancedStruct());
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
		
		OwningAbilityComponent->CancelAbility(SubAbility.AbilityID, StopContext);
	}
	
	CleanUpAbility();
}

FGuid USimpleGameplayAbility::ActivateSubAbility(
	TSubclassOf<USimpleGameplayAbility> AbilityClass,
	FAbilityContextCollection ActivationContext,
	ESubAbilityCancellationPolicy CancellationPolicy)
{
	const FGuid SubAbilityID = FGuid::NewGuid();

	FActivatedSubAbility SubAbility;
	SubAbility.AbilityID = SubAbilityID;
	SubAbility.CancellationPolicy = CancellationPolicy;
	ActivatedSubAbilities.Add(SubAbility);

	OwningAbilityComponent->ActivateAbilityWithID(SubAbilityID, AbilityClass, ActivationContext, EAbilityActivationPolicyOverride::ForceLocalOnly);

	return SubAbilityID;
}

AActor* USimpleGameplayAbility::GetAvatarActor() const
{
	if (!OwningAbilityComponent)
	{
		SIMPLE_LOG(OwningAbilityComponent,
			   FString::Printf(TEXT("Owning ability component is null for ability %s"), *GetName()));
		return nullptr;
	}

	if (!OwningAbilityComponent->GetAvatarActor())
	{
		SIMPLE_LOG(OwningAbilityComponent,
		           FString::Printf(
			           TEXT("Owning ability component has no avatar actor for ability %s"), *GetName()));
		return nullptr;
	}

	return OwningAbilityComponent->GetAvatarActor();
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
		SIMPLE_LOG(OwningAbilityComponent,
			   FString::Printf(TEXT("Avatar actor %s is not of type %s"), *AvatarActor->GetName(),
							   *AvatarClass->GetName()));
		IsValid = false;
		return AvatarActor;
	}
	
	IsValid = true;
	return AvatarActor;
}

void USimpleGameplayAbility::ApplyAttributeModifierToTarget(
	USimpleGameplayAbilityComponent* TargetComponent,
	const TSubclassOf<USimpleAttributeModifier> ModifierClass,
	const float Magnitude,
	const FAbilityContextCollection ModifierContext,
	FGuid& ModifierID)
{
	OwningAbilityComponent->ApplyAttributeModifierToTarget(TargetComponent, ModifierClass, Magnitude, ModifierContext, ModifierID);
}

double USimpleGameplayAbility::GetActivationTime() const
{
	return ActivationTime;
}

double USimpleGameplayAbility::GetActivationDelay() const
{
	return OwningAbilityComponent->GetServerTime() - GetActivationTime();
}

void USimpleGameplayAbility::SendEvent(
	const FGameplayTag EventTag,
	const FGameplayTag DomainTag,
	const FInstancedStruct EventContext,
	const ESimpleEventReplicationPolicy ReplicationPolicy)
{
	OwningAbilityComponent->SendEvent(EventTag, DomainTag, EventContext, this, {}, ReplicationPolicy);
}
