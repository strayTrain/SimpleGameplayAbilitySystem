// Fill out your copyright notice in the Description page of Project Settings.


#include "WaitForStructAttributeChange.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

UWaitForStructAttributeChange* UWaitForStructAttributeChange::WaitForStructAttributeChange(UObject* WorldContextObject,
                                                                                           USimpleGameplayAbilityComponent* AttributeOwner,
                                                                                           FGameplayTag AttributeTag,
                                                                                           bool OnlyTriggerOnce)
{
	UWaitForStructAttributeChange* Task = NewObject<UWaitForStructAttributeChange>();

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World || !AttributeOwner)
	{
		UE_LOG(LogTemp, Error, TEXT("UWaitForStructAttributeChange::WaitForStructAttributeChange: Invalid World or AttributeOwner"));
		Task->SetReadyToDestroy();
		return nullptr;
	}
	
	Task->AttributeOwner = AttributeOwner;
	Task->Attribute = AttributeTag;
	Task->WorldContext = World;

	Task->ShouldOnlyTriggerOnce = OnlyTriggerOnce;
	Task->RegisterWithGameInstance(World);
	
	return Task;
	
}

void UWaitForStructAttributeChange::Activate()
{
	if (USimpleEventSubsystem* EventSubsystem = WorldContext->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		FGameplayTagContainer EventFilter;
		EventFilter.AddTagFast(FDefaultTags::StructAttributeValueChanged);

		FGameplayTagContainer DomainFilter;
		TArray<UScriptStruct*> PayloadFilter;
		
		TArray<AActor*> SenderFilter;
		SenderFilter.Add(AttributeOwner->GetOwner());

		EventCallbackDelegate.BindDynamic(this, &UWaitForStructAttributeChange::OnSimpleEventReceived);
		EventID = EventSubsystem->ListenForEvent(this, ShouldOnlyTriggerOnce, EventFilter, DomainFilter, EventCallbackDelegate, PayloadFilter, SenderFilter);
		EventSubsystem->OnEventSubscriptionRemoved.AddDynamic(this, &UWaitForStructAttributeChange::OnEventSubscriptionRemoved);
	}
	else
	{
		SetReadyToDestroy();
	}
}

void UWaitForStructAttributeChange::OnSimpleEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	OnStructAttributeChanged.Broadcast(DomainTag, Payload);
	
	if (ShouldOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForStructAttributeChange::OnEventSubscriptionRemoved(FGuid SubscriptionID)
{
	if (EventID == SubscriptionID)
	{
		SetReadyToDestroy();
	}
}