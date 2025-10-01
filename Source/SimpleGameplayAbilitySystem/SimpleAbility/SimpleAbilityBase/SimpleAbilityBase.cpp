#include "SimpleAbilityBase.h"

bool USimpleAbilityBase::ActivateAbility(FInstancedStruct ActivationContext)
{
	AbilityContext = ActivationContext;
	
	if (!CanActivateInternal())
	{
		OnActivationFailed.Broadcast(this);
		return false;
	}

	IsActive = true;
	PreActivateInternal();
	OnPreActivate();
	OnActivate(AbilityContext);
	OnActivationSuccess.Broadcast(this);
	return true;
}

void USimpleAbilityBase::EndAbility(FGameplayTag EndStatus, FInstancedStruct EndContext)
{
	IsActive = false;
	AbilityEndedInternal(EndContext, false);
	OnEnd(EndStatus, EndContext);
	OnAbilityEnded.Broadcast(this, EndStatus, EndContext);
	OnCleanUpAbility();
}

void USimpleAbilityBase::CancelAbility(FGameplayTag EndStatus, FInstancedStruct EndContext)
{
	IsActive = false;
	AbilityEndedInternal(EndContext, true);
	OnCancel(EndStatus, EndContext);
	OnAbilityCancelled.Broadcast(this, EndStatus, EndContext);
	OnCleanUpAbility();
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
