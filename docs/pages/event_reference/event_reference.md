---
title: Event Reference
layout: home
nav_order: 4
---

# SimpleGAS Event Reference

This page documents all built-in events that SimpleGAS broadcasts through the `SimpleEventSubsystem`. You can listen for these events in your game to respond to various system changes.  
You can read about how to listen for events on the [event system concept page](../concepts/event_system/event_subsystem.html#listening-for-events){:target="_blank"}.


## Ability Component Events

| Event Tag | Domain Tag | Description | Payload Type |
|-----------|------------|-------------|--------------|
| `SimpleGAS.Events.AbilityComponent.GameplayTagAdded` | The tag that was added | Fired when a gameplay tag is added to an ability component | None |
| `SimpleGAS.Events.AbilityComponent.GameplayTagRemoved` | The tag that was removed | Fired when a gameplay tag is removed from an ability component | None |

## Ability Events

| Event Tag | Domain Tag | Description | Payload Type |
|-----------|------------|-------------|--------------|
| `SimpleGAS.Events.Ability.Activated` | `SimpleGAS.Domains.Ability.Local` or `SimpleGAS.Domains.Ability.Authority` | Fired when an ability is successfully activated | [FAbilityActivationEvent](#fabilityactivationevent) |
| `SimpleGAS.Events.Ability.Ended` | `SimpleGAS.Domains.Ability.Local` or `SimpleGAS.Domains.Ability.Authority` | Fired when an ability ends for any reason | [FSimpleAbilityEndedEvent](#fsimpleabilityendedevent) |

## Attribute Events

### Float Attributes

| Event Tag | Domain Tag | Description | Payload Type |
|-----------|------------|-------------|--------------|
| `SimpleGAS.Events.Attributes.Added.Float` | The added attribute tag | Fired when a float attribute is added | None |
| `SimpleGAS.Events.Attributes.Removed.Float` | The removed attribute tag | Fired when a float attribute is removed | None |
| `SimpleGAS.Events.Attributes.Changed.FloatBaseValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the base value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatMinBaseValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the min base value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatMaxBaseValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the max base value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatCurrentValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the current value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatMinCurrentValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the min current value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatMaxCurrentValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the max current value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |

### Struct Attributes

| Event Tag | Domain Tag | Description | Payload Type |
|-----------|------------|-------------|--------------|
| `SimpleGAS.Events.Attributes.Added.Struct` | The added attribute tag | Fired when a struct attribute is added | [FInstancedStruct](#finstancedstruct) |
| `SimpleGAS.Events.Attributes.Changed.Struct` | The added attribute tag | Fired when a struct attribute is changed | [FInstancedStruct](#finstancedstruct) |
| `SimpleGAS.Events.Attributes.Removed.Struct` | The added attribute tag | Fired when a struct attribute is removed | [FInstancedStruct](#finstancedstruct) |

## Domain Tags

Domain tags are used as a secondary categorization for events and can be useful for filtering events when listening. The primary domain tags are:

| Domain Tag | Description |
|------------|-------------|
| `SimpleGAS.Domains.Ability.Local` | Events that originated on the local client |
| `SimpleGAS.Domains.Ability.Authority` | Events that originated on the server (authoritative) |
| `SimpleGAS.Domains.Attribute.Local` | Attribute events that originated on the local client |
| `SimpleGAS.Domains.Attribute.Authority` | Attribute events that originated on the server |

## Payload Struct Definitions

### FAbilityActivationEvent

```cpp
struct FAbilityActivationEvent
{
	FGuid AbilityID;                                  // The ID of the ability that was activated
	TSubclassOf<USimpleGameplayAbility> AbilityClass; // The class of the ability that was activated
	FInstancedStruct AbilityContext;                  // The context data passed to the ability when it was activated
	bool WasActivatedSuccessfully;                    // Whether the ability was activated successfully
	float ActivationTimeStamp;                        // The time when the ability was activated (in seconds, according to the server time)
};
```

### FSimpleAbilityEndedEvent

```cpp
struct FSimpleAbilityEndedEvent
{
    FGuid AbilityID;                  // The ID of the ability that ended
    FGameplayTag EndStatusTag;        // A tag describing how the ability ended
    FInstancedStruct EndingContext;   // Optional context data about how/why the ability ended
    EAbilityStatus NewAbilityStatus;  // The new status of the ability
    bool WasCancelled;                // Whether the ability was cancelled or ended normally
};
```

### FFloatAttributeModification

```cpp
struct FFloatAttributeModification
{
    FGameplayTag AttributeTag;        // The tag of the attribute that was modified
    EAttributeValueType ValueType;    // Which value of the attribute was modified (base, current, etc.)
    float NewValue;                   // The new value of the attribute
};
```

### FAttributeModifierResult

```cpp
struct FAttributeModifierResult
{
    USimpleGameplayAbilityComponent* Instigator;  // Component that applied the modifier
    USimpleGameplayAbilityComponent* Target;      // Component that received the modifier
    TArray<FAbilitySideEffect> AppliedAbilitySideEffects;         // Abilities activated as side effects
    TArray<FEventSideEffect> AppliedEventSideEffects;             // Events sent as side effects
    TArray<FAttributeModifierSideEffect> AppliedAttributeModifierSideEffects;  // Modifiers applied as side effects
};
```

### FInstancedStruct

`FInstancedStruct` is a container that can hold any UE struct type. For struct attribute events, the payload will be the actual struct value of the attribute. The type will depend on what struct type your attribute is using.