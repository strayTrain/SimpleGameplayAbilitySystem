#include "SimpleSubAbility.h"

#include "SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"
#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/Module/SimpleGameplayAbilitySystem.h"
#include "SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h"

void USimpleSubAbility::Initialize(USimpleGameplayAbility* ParentAbility, const FGuid NewParentAbilityID)
{
	ParentAbilityInstance = ParentAbility;
	ParentAbilityID = NewParentAbilityID;
}

bool USimpleSubAbility::CanActivateInternal()
{
	// Check RequiredContextType
	if (RequiredContextType)
	{
		if (!AbilityContext.IsValid() || AbilityContext.GetScriptStruct() != RequiredContextType)
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleSubAbility::CanActivateInternal]: Context provided to the ability doesn't match required context type %s"), *RequiredContextType->GetName()));
			return false;
		}
	}

	// Check AvatarTypeFilter
	if (AvatarTypeFilter.Num() > 0)
	{
		AActor* AvatarActor = GetAvatarActor();
		if (!AvatarActor)
		{
			return false;
		}

		bool bMatchesFilter = false;
		for (TSubclassOf<AActor> AllowedClass : AvatarTypeFilter)
		{
			if (AvatarActor->IsA(AllowedClass))
			{
				bMatchesFilter = true;
				break;
			}
		}

		if (!bMatchesFilter)
		{
			SIMPLE_LOG(this, FString::Printf(TEXT("[USimpleSubAbility::CanActivateInternal]: Avatar actor %s doesn't pass the AvatarTypeFilter in sub ability %s"), *AvatarActor->GetName(), *GetName()));
			return false;
		}
	}

	return true;
}

AActor* USimpleSubAbility::GetAvatarActor() const
{
	return ParentAbilityInstance->GetAvatarActor();
}

AActor* USimpleSubAbility::GetAvatarActorAs(TSubclassOf<AActor> AvatarClass, EGetAvatarActorResult& Result)
{
	return ParentAbilityInstance->GetAvatarActorAs(AvatarClass, Result);
}

USimpleGameplayAbilityComponent* USimpleSubAbility::GetParentAbilityComponent() const
{
	return ParentAbilityInstance ? ParentAbilityInstance->GetAbilityComponent() : nullptr;
}

void USimpleSubAbility::SendEventToServer(FGameplayTag EventTag, FInstancedStruct EventContext)
{
	if (!ParentAbilityInstance)
	{
		return;
	}

	ParentAbilityInstance->GetAbilityComponent()->SendEventToServer(
		EventTag,
		FDefaultTags::DomainAbility(),
		EventContext,
		this,
		TArray<UObject*>());
}

void USimpleSubAbility::SendEventToClient(FGameplayTag EventTag, FInstancedStruct EventContext)
{
	if (!ParentAbilityInstance)
	{
		return;
	}

	ParentAbilityInstance->GetAbilityComponent()->SendEventToClient(
		EventTag,
		FDefaultTags::DomainAbility(),
		EventContext,
		this,
		TArray<UObject*>());
}

void USimpleSubAbility::SendEvent(FGameplayTag EventTag, FInstancedStruct EventContext)
{
	if (!ParentAbilityInstance)
	{
		return;
	}

	ParentAbilityInstance->GetAbilityComponent()->SendEvent(
		EventTag,
		FDefaultTags::DomainAbility(),
		EventContext,
		this,
		TArray<UObject*>());
}
