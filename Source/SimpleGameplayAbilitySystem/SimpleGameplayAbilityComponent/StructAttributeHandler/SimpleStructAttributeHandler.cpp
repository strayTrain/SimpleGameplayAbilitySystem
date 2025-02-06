// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleStructAttributeHandler.h"

#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "SimpleGameplayAbilitySystem/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h"

void USimpleStructAttributeHandler::Initialize(USimpleGameplayAbilityComponent* InAbilityComponent)
{
	AbilityComponent = InAbilityComponent;
}

void USimpleStructAttributeHandler::SendEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, ESimpleEventReplicationPolicy ReplicationPolicy) const
{
	if (AbilityComponent)
	{
		AbilityComponent->SendEvent(EventTag, DomainTag, Payload, AbilityComponent->AvatarActor, ReplicationPolicy);
	}
}

FInstancedStruct USimpleStructAttributeHandler::OnInitializeStruct_Implementation()
{
	/* When creating a new instance of this struct attribute, you can initialize the struct members here
		Example implementation
		----------------------
		Let's use this struct as the attribute type:
		struct FloatStringStruct
		{
			float FloatValue;
			FString StringValue;
		};

		FloatingStringStruct StructValue;
		StructValue.FloatValue = 0.0f;
		StructValue.StringValue = "Default String Value";

		return FInstancedStruct::Make(StructValue);
	*/

	return FInstancedStruct();
}

void USimpleStructAttributeHandler::OnStructChanged_Implementation(FInstancedStruct OldStruct,
                                                                   FInstancedStruct NewStruct)
{
	/* Compare the old struct and the new struct for changes and send custom events here

	Example implementation
	----------------------
	
	Let's use this struct as the attribute type:
	struct FloatStringStruct
	{
		float FloatValue;
		FString StringValue;
	};

	const FloatStringStruct* OldStruct = OldStruct.GetPtr<FloatStringStruct>();
	const FloatStringStruct* NewStruct = NewStruct.GetPtr<FloatStringStruct>();

	if (OldStruct && NewStruct)
	{
		USimpleEventSubsystem* EventSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<USimpleEventSubsystem>();

		if (!EventSubsystem)
		{
			return;
		}
		
		if (OldStruct->FloatValue != NewStruct->FloatValue)
		{
			EventSubsystem->SendEvent(
				FloatValueChangedEventTag,
				YouDomainTag,
				YouFInstancedStructPayload,
				OptionalSenderActor
			);
		}

		if (OldStruct->StringValue != NewStruct->StringValue)
		{
			EventSubsystem->SendEvent(
				StringValueChangedEventTag,
				YouDomainTag,
				YouFInstancedStructPayload,
				OptionalSenderActor
			);
		}
	}
	
	*/
}
