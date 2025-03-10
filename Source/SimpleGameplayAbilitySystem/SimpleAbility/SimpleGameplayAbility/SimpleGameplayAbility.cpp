#include "SimpleGameplayAbility.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

bool USimpleGameplayAbility::CanActivate_Implementation(FInstancedStruct ActivationContext)
{
    return true;
}

void USimpleGameplayAbility::PreActivate_Implementation(FInstancedStruct ActivationContext)
{
}

bool USimpleGameplayAbility::CanCancel_Implementation()
{
    return true;
}

bool USimpleGameplayAbility::ActivateAbility(const FGuid AbilityID, FInstancedStruct ActivationContext)
{
    AbilityInstanceID = AbilityID;
    
    if (!MeetsDefaultRequirements(ActivationContext) || !CanActivate(ActivationContext))
    {
        OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::EndedActivationFailed);
        return false;    
    }

    for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
    {
        OwningAbilityComponent->AddGameplayTag(TempTag, ActivationContext);
    }

    for (const FGameplayTag& PermTag : PermanentlyAppliedTags)
    {
        OwningAbilityComponent->AddGameplayTag(PermTag, ActivationContext);
    }

    OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, EAbilityStatus::ActivationSuccess);
    CachedActivationContext = ActivationContext;

    PreActivate(ActivationContext);
    bIsAbilityActive = true;
    OnActivate(ActivationContext);
    
    return true;
}

FGuid USimpleGameplayAbility::ActivateSubAbility(const TSubclassOf<USimpleGameplayAbility> AbilityClass,
    const FInstancedStruct ActivationContext, const bool CancelIfParentEnds, const bool CancelIfParentCancels)
{
    const FGuid SubAbilityID = FGuid::NewGuid();

    if (CancelIfParentEnds)
    {
        EndOnEndedSubAbilities.Add(SubAbilityID);
    }

    if (CancelIfParentCancels)
    {
        EndOnCancelledSubAbilities.Add(SubAbilityID);
    }
    
    OwningAbilityComponent->ActivateAbilityWithID(SubAbilityID, AbilityClass, ActivationContext, false, true, EAbilityActivationPolicy::LocalOnly);
    
    return SubAbilityID;
}

void USimpleGameplayAbility::OnTick_Implementation(float DeltaTime)
{
    
}

void USimpleGameplayAbility::EndAbility(const FGameplayTag EndStatus, const FInstancedStruct EndingContext)
{
    if (!bIsAbilityActive)
    {
        return;
    }
    
    OnEnd(EndStatus, EndingContext, false);
    EndAbilityInternal(EndStatus, EndingContext, false);
}

void USimpleGameplayAbility::CancelAbility(const FGameplayTag CancelStatus, const FInstancedStruct CancelContext)
{
    if (!bIsAbilityActive || !CanCancel())
    {
        return;
    }
    
    OnEnd(CancelStatus, CancelContext, true);
    EndAbilityInternal(CancelStatus, CancelContext, true);
}

void USimpleGameplayAbility::EndAbilityInternal(FGameplayTag Status, FInstancedStruct Context, bool WasCancelled)
{
    for (const FGameplayTag& TempTag : TemporarilyAppliedTags)
    {
        OwningAbilityComponent->RemoveGameplayTag(TempTag, Context);
    }

    const EAbilityStatus StatusToSet = WasCancelled ? EAbilityStatus::EndedCancelled : EAbilityStatus::EndedSuccessfully;
    const TArray<FGuid>& AbilitiesToCancel = WasCancelled ? EndOnCancelledSubAbilities : EndOnEndedSubAbilities;
    
    for (const FGuid& SubAbilityID : AbilitiesToCancel)
    {
        OwningAbilityComponent->CancelAbility(SubAbilityID, Context);
    }
    
    OwningAbilityComponent->ChangeAbilityStatus(AbilityInstanceID, StatusToSet);
    bIsAbilityActive = false;
    
    if (InstancingPolicy == EAbilityInstancingPolicy::MultipleInstances)
    {
        OwningAbilityComponent->RemoveInstancedAbility(this);
    }
    
    if (IsProxyAbility && !OwningAbilityComponent->HasAuthority())
    {
        return;
    }
    
    OwningAbilityComponent->SetAbilityStateEndingContext(AbilityInstanceID, Status, Context);
}

AActor* USimpleGameplayAbility::GetAvatarActorAs(TSubclassOf<AActor> AvatarClass, bool& IsValid) const
{
    if (AActor* AvatarActor = OwningAbilityComponent->GetAvatarActor())
    {
        if (AvatarActor->IsA(AvatarClass))
        {
            IsValid = true;
            return AvatarActor;
        }

        SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("Avatar actor %s is not of type %s"), *AvatarActor->GetName(), *AvatarClass->GetName()));
        IsValid = false;
        return AvatarActor;
    }

    UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s owning component has no avatar actor set. Did you remember to call SetAvatarActor?"), *AbilityInstanceID.ToString());
    return nullptr;
}

void USimpleGameplayAbility::ApplyAttributeModifierToTarget(USimpleGameplayAbilityComponent* TargetComponent,
    TSubclassOf<USimpleAttributeModifier> ModifierClass, FInstancedStruct Context, FGuid& ModifierID)
{
    OwningAbilityComponent->ApplyAttributeModifierToTarget(TargetComponent, ModifierClass, Context, ModifierID);
}

bool USimpleGameplayAbility::IsAbilityActive() const
{
    return bIsAbilityActive;
}

double USimpleGameplayAbility::GetActivationTime() const
{
    bool WasStateFound;
    const FAbilityState AbilityState = OwningAbilityComponent->FindAbilityState(AbilityInstanceID, WasStateFound);

    if (WasStateFound)
    {
        return AbilityState.ActivationTimeStamp;
    }

    UE_LOG(LogSimpleGAS, Warning, TEXT("Ability with ID %s not found in AbilityState array"), *AbilityInstanceID.ToString());
    return 0;
}

double USimpleGameplayAbility::GetActivationDelay() const
{
    return OwningAbilityComponent->GetServerTime() - GetActivationTime();
}

FInstancedStruct USimpleGameplayAbility::GetActivationContext() const
{
    return CachedActivationContext;
}

bool USimpleGameplayAbility::WasActivatedOnServer() const
{
    return OwningAbilityComponent->HasAuthority() && !IsProxyAbility;
}

bool USimpleGameplayAbility::WasActivatedOnClient() const
{
    return (!OwningAbilityComponent->HasAuthority() || OwningAbilityComponent->GetNetMode() == NM_ListenServer) && !IsProxyAbility;
}

UWorld* USimpleGameplayAbility::GetWorld() const
{
    if (OwningAbilityComponent)
    {
        return OwningAbilityComponent->GetWorld();
    }

    return nullptr;
}

void USimpleGameplayAbility::Tick(float DeltaTime)
{
    OnTick(DeltaTime);
}

bool USimpleGameplayAbility::IsTickable() const
{
    UWorld* World = GetWorld();
    return CanTick && IsAbilityActive() && World != nullptr;
}

TStatId USimpleGameplayAbility::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(USimpleGameplayAbility, STATGROUP_Tickables);
}

ETickableTickType USimpleGameplayAbility::GetTickableTickType() const
{
    // Only tick during gameplay, not in editor
    return ETickableTickType::Conditional;
}

bool USimpleGameplayAbility::MeetsDefaultRequirements(FInstancedStruct& ActivationContext) const
{
    if (ActivationBlockingTags.Num() > 0)
    {
        for (const FGameplayTag& BlockingTag : ActivationBlockingTags)
        {
            if (OwningAbilityComponent->GameplayTags.HasTagExact(BlockingTag))
            {
                UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s blocked by tag %s"), *GetName(), *BlockingTag.ToString());
                return false;
            }
        }
    }

    if (ActivationRequiredTags.Num() > 0)
    {
        for (const FGameplayTag& RequiredTag : ActivationRequiredTags)
        {
            if (!OwningAbilityComponent->GameplayTags.HasTagExact(RequiredTag))
            {
                UE_LOG(LogSimpleGAS, Warning, TEXT("Ability %s requires tag %s"), *GetName(), *RequiredTag.ToString());
                return false;
            }
        }
    }

    if (RequiredContextType)
    {
        if (!ActivationContext.IsValid())
        {
            SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("Ability %s requires ActivationContext that contains struct type %s. No ActivationContext was provided."), *GetName(), *RequiredContextType->GetName()));
            return false;
        }

        if (ActivationContext.IsValid() && RequiredContextType != ActivationContext.GetScriptStruct())
        {
            SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("Ability %s requires ActivationContext that contains struct type %s. The struct that was passed in is type %s"), *GetName(), *RequiredContextType->GetName(), *ActivationContext.GetScriptStruct()->GetName()));
            return false;
        }
    }
    
    if (AvatarTypeFilter.Num() > 0)
    {
        const AActor* AvatarActor = OwningAbilityComponent->GetAvatarActor();

        if (!AvatarActor)
        {
            SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("Ability %s requires an avatar actor to activate"), *GetName()));
            return false;
        }

        if (!AvatarTypeFilter.Contains(AvatarActor->GetClass()))
        {
            SIMPLE_LOG(OwningAbilityComponent, FString::Printf(TEXT("Ability %s requires an avatar actor of type %s"), *GetName(), *AvatarTypeFilter[0]->GetName()));
            return false;
        }    
    }
    
    return true;
}

void USimpleGameplayAbility::ClientResolvePastState(FGameplayTag StateTag, FSimpleAbilitySnapshot AuthorityState, FSimpleAbilitySnapshot PredictedState)
{
    // First we do a deep comparison of the underlying FInstancedStructs representing the snapshot data. If they're the same, we don't need to do anything.
    if (AuthorityState.StateData == PredictedState.StateData)
    {
        SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleGameplayAbility::ClientResolvePastState]: No diverging state detected for ability snapshot %s. No action taken."), *StateTag.ToString()));
        return;
    }
    
    // Invoke any registered callbacks for this state
    if (SnapshotResolveCallbacks.Contains(PredictedState.SequenceNumber))
    {
        FOnSnapshotResolved& Callback = SnapshotResolveCallbacks[PredictedState.SequenceNumber];
        if (Callback.IsBound())
        {
            Callback.Execute(StateTag, AuthorityState, PredictedState);
            SnapshotResolveCallbacks.Remove(PredictedState.SequenceNumber);
        }
    }
}