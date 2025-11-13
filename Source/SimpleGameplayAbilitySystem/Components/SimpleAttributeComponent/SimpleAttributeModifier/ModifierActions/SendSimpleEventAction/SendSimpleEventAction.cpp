//  Copyright 2025 Ahmed Elgoni


#include "SendSimpleEventAction.h"

#include "SimpleGameplayAbilitySystem/BlueprintFunctionLibraries/FunctionSelectors/FunctionSelectors.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventSubsystem.h"

bool USendSimpleEventAction::CanApply_Implementation() const
{
	// Get the SimpleEventSubsystem and dispatch the event
	return GetWorld() && GetWorld()->GetGameInstance();
}

void USendSimpleEventAction::ApplyAction_Implementation()
{
	FInstancedStruct PayloadStruct = FInstancedStruct();

	// If a PayloadFunction is defined, execute it to get the payload
	if (PayloadFunction.GetMemberName() != NAME_None)
	{
		UFunctionSelectors::GetStructContext(OwningModifier, PayloadFunction, PayloadStruct);
	}

	// Get the SimpleEventSubsystem and dispatch the event
	if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
	{
		if (USimpleEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<USimpleEventSubsystem>())
		{
			EventSubsystem->SendEvent(EventTag, DomainTag, PayloadStruct, OwningModifier, {});
		}
	}
}
