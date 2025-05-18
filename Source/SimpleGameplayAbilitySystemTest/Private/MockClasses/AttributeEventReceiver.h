#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "SimpleGameplayAbilitySystem/SimpleEventSubsystem/SimpleEventTypes.h" // For FInstancedStruct
#include "AttributeEventReceiver.generated.h"

UCLASS()
class UAttributeEventReceiver : public UObject
{
    GENERATED_BODY()
public:
    bool bEventFired = false;
    FGameplayTag ExpectedEventTag;
    FGameplayTag ExpectedDomainTag;
    AActor* ExpectedSenderActor = nullptr; // Store the Actor, not the component directly

    UFUNCTION()
    void HandleEvent(FGameplayTag EventTag, FGameplayTag DomainTag, FInstancedStruct Payload, UObject* Sender)
    {
        if (EventTag == ExpectedEventTag && DomainTag == ExpectedDomainTag)
        {
            if (ExpectedSenderActor && Sender == ExpectedSenderActor)
            {
                bEventFired = true;
            }
            else if (!ExpectedSenderActor)
            {
                bEventFired = true;
            }
        }
    }
};