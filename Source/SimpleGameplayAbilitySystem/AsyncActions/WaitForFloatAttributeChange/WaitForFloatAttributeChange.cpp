// Fill out your copyright notice in the Description page of Project Settings.


#include "WaitForFloatAttributeChange.h"

#include "SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubSystem.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

UWaitForFloatAttributeChange* UWaitForFloatAttributeChange::WaitForFloatAttributeChange(UObject* WorldContextObject,
                                                                                        USimpleGameplayAbilityComponent* AttributeOwner, FGameplayTag AttributeTag,
                                                                                        bool OnlyTriggerOnce,
                                                                                        bool ListenForBaseValueChange,
                                                                                        bool ListenForCurrentValueChange,
                                                                                        bool ListenForMaxBaseValueChange,
                                                                                        bool ListenForMinBaseValueChange,
                                                                                        bool ListenForMaxCurrentValueChange,
                                                                                        bool ListenForMinCurrentValueChange)
{
	UWaitForFloatAttributeChange* Task = NewObject<UWaitForFloatAttributeChange>();

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World || !AttributeOwner)
	{
		UE_LOG(LogTemp, Error, TEXT("UWaitForFloatAttributeChange::WaitForFloatAttributeChange: Invalid World or AttributeOwner"));
		Task->SetReadyToDestroy();
		return nullptr;
	}
	
	Task->AttributeOwner = AttributeOwner;
	Task->Attribute = AttributeTag;
	Task->WorldContext = World;

	Task->ShouldOnlyTriggerOnce = OnlyTriggerOnce;
	Task->ShouldListenForBaseValueChange = ListenForBaseValueChange;
	Task->ShouldListenForCurrentValueChange = ListenForCurrentValueChange;
	Task->ShouldListenForMaxBaseValueChange = ListenForMaxBaseValueChange;
	Task->ShouldListenForMinBaseValueChange = ListenForMinBaseValueChange;
	Task->ShouldListenForMaxCurrentValueChange = ListenForMaxCurrentValueChange;
	Task->ShouldListenForMinCurrentValueChange = ListenForMinCurrentValueChange;

	Task->RegisterWithGameInstance(World);
	
	return Task;
	
}

void UWaitForFloatAttributeChange::Activate()
{
	if (USimpleEventSubsystem* EventSubsystem = WorldContext->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>())
	{
		EventCallbackDelegate.BindDynamic(this, &UWaitForFloatAttributeChange::OnSimpleEventReceived);

		FGameplayTagContainer EventFilter;
		if (ShouldListenForBaseValueChange)
		{
			EventFilter.AddTagFast(FDefaultTags::FloatAttributeBaseValueChanged);
		}

		if (ShouldListenForCurrentValueChange)
		{
			EventFilter.AddTagFast(FDefaultTags::FloatAttributeCurrentValueChanged);
		}

		if (ShouldListenForMaxBaseValueChange)
		{
			EventFilter.AddTagFast(FDefaultTags::FloatAttributeMaxBaseValueChanged);
		}

		if (ShouldListenForMinBaseValueChange)
		{
			EventFilter.AddTagFast(FDefaultTags::FloatAttributeMinBaseValueChanged);
		}

		if (ShouldListenForMaxCurrentValueChange)
		{
			EventFilter.AddTagFast(FDefaultTags::FloatAttributeMaxCurrentValueChanged);
		}

		if (ShouldListenForMinCurrentValueChange)
		{
			EventFilter.AddTagFast(FDefaultTags::FloatAttributeMinCurrentValueChanged);
		}

		FGameplayTagContainer DomainFilter;
		DomainFilter.AddTagFast(FDefaultTags::LocalDomain);
		DomainFilter.AddTagFast(FDefaultTags::AuthorityDomain);
		
		TArray<UScriptStruct*> PayloadFilter;
		PayloadFilter.Add(FFloatAttributeModification::StaticStruct());
		
		TArray<AActor*> SenderFilter;
		SenderFilter.Add(AttributeOwner->GetOwner());
		
		EventID = EventSubsystem->ListenForEvent(this, ShouldOnlyTriggerOnce, EventFilter, DomainFilter, EventCallbackDelegate, PayloadFilter, SenderFilter);
		EventSubsystem->OnEventSubscriptionRemoved.AddDynamic(this, &UWaitForFloatAttributeChange::OnEventSubscriptionRemoved);
	}
	else
	{
		SetReadyToDestroy();
	}
}

void UWaitForFloatAttributeChange::OnSimpleEventReceived(FGameplayTag AbilityTag, FGameplayTag DomainTag, FInstancedStruct Payload)
{
	const FFloatAttributeModification AttributeModification = Payload.Get<FFloatAttributeModification>();

	if (AttributeModification.AttributeTag != Attribute)
	{
		return;
	}
	
	OnAttributeChanged.Broadcast(AttributeModification.ValueType, AttributeModification.NewValue);
	
	if (ShouldOnlyTriggerOnce)
	{
		SetReadyToDestroy();
	}
}

void UWaitForFloatAttributeChange::OnEventSubscriptionRemoved(FGuid SubscriptionID)
{
	if (EventID == SubscriptionID)
	{
		SetReadyToDestroy();
	}
}
