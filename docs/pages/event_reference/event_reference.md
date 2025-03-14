---
title: Event Reference
layout: home
nav_order: 4
---

# SimpleGAS Event Reference

This page documents all built-in events that SimpleGAS broadcasts through the `SimpleEventSubsystem`. You can listen for these events in your game to respond to various system changes.

## Ability Component Events

| Event Tag | Domain Tag | Description | Payload Type |
|-----------|------------|-------------|--------------|
| `SimpleGAS.Events.AbilityComponent.GameplayTagAdded` | Event Tag | Fired when a gameplay tag is added to an ability component | Variable |
| `SimpleGAS.Events.AbilityComponent.GameplayTagRemoved` | Event Tag | Fired when a gameplay tag is removed from an ability component | Variable |

## Ability Events

| Event Tag | Domain Tag | Description | Payload Type |
|-----------|------------|-------------|--------------|
| `SimpleGAS.Events.Ability.Added` | None | Fired when an ability is granted to an ability component | None |
| `SimpleGAS.Events.Ability.Removed` | None | Fired when an ability is revoked from an ability component | None |
| `SimpleGAS.Events.Ability.Activated` | None | Fired when an ability is successfully activated | None |
| `SimpleGAS.Events.Ability.Ended` | `SimpleGAS.Domains.Ability.Local` or `SimpleGAS.Domains.Ability.Authority` | Fired when an ability ends for any reason | [FSimpleAbilityEndedEvent](#fsimpleabilityendedevent) |
| `SimpleGAS.Events.Ability.Ended.Success` | `SimpleGAS.Domains.Ability.Local` or `SimpleGAS.Domains.Ability.Authority` | Fired when an ability ends successfully | [FSimpleAbilityEndedEvent](#fsimpleabilityendedevent) |
| `SimpleGAS.Events.Ability.Ended.Cancel` | `SimpleGAS.Domains.Ability.Local` or `SimpleGAS.Domains.Ability.Authority` | Fired when an ability is cancelled | [FSimpleAbilityEndedEvent](#fsimpleabilityendedevent) |
| `SimpleGAS.Events.Ability.SnapshotTaken` | `SimpleGAS.Domains.Ability.Local` or `SimpleGAS.Domains.Ability.Authority` | Fired when a state snapshot is taken by an ability | [FSimpleAbilitySnapshot](#fsimpleabilitysnapshot) |
| `SimpleGAS.Events.Ability.WaitForAbilityEnded` | Event Status Tag | Used by the async action `WaitForAbility` to signal when a sub-ability has ended | [FSimpleAbilityEndedEvent](#fsimpleabilityendedevent) |

## Attribute Events

### Float Attributes

| Event Tag | Domain Tag | Description | Payload Type |
|-----------|------------|-------------|--------------|
| `SimpleGAS.Events.Attributes.Added.Float` | None | Fired when a float attribute is added | None |
| `SimpleGAS.Events.Attributes.Removed.Float` | None | Fired when a float attribute is removed | None |
| `SimpleGAS.Events.Attributes.Changed.FloatBaseValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the base value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatMinBaseValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the min base value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatMaxBaseValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the max base value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatCurrentValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the current value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatMinCurrentValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the min current value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |
| `SimpleGAS.Events.Attributes.Changed.FloatMaxCurrentValue` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when the max current value of a float attribute changes | [FFloatAttributeModification](#ffloatattributemodification) |

### Struct Attributes

| Event Tag | Domain Tag | Description | Payload Type |
|-----------|------------|-------------|--------------|
| `SimpleGAS.Events.Attributes.Added.Struct` | None | Fired when a struct attribute is added | [FInstancedStruct](#finstancedstruct) |
| `SimpleGAS.Events.Attributes.Changed.Struct` | Modification Event Tag | Fired when a struct attribute is changed | [FInstancedStruct](#finstancedstruct) |
| `SimpleGAS.Events.Attributes.Removed.Struct` | None | Fired when a struct attribute is removed | None |

## Attribute Modifier Events

| Event Tag | Domain Tag | Description | Payload Type |
|-----------|------------|-------------|--------------|
| `SimpleGAS.Events.AttributeModifer.Applied` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when a modifier is initially applied | [FAttributeModifierResult](#fattributemodifierresult) |
| `SimpleGAS.Events.AttributeModifer.Ticked` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when a duration modifier ticks | [FAttributeModifierResult](#fattributemodifierresult) |
| `SimpleGAS.Events.AttributeModifer.Ended` | `SimpleGAS.Domains.Attribute.Local` or `SimpleGAS.Domains.Attribute.Authority` | Fired when a duration modifier ends | [FAttributeModifierResult](#fattributemodifierresult) |

## Domain Tags

Domain tags are used as a secondary categorization for events and can be useful for filtering events when listening. The primary domain tags are:

| Domain Tag | Description |
|------------|-------------|
| `SimpleGAS.Domains.Ability.Local` | Events that originated on the local client |
| `SimpleGAS.Domains.Ability.Authority` | Events that originated on the server (authoritative) |
| `SimpleGAS.Domains.Attribute.Local` | Attribute events that originated on the local client |
| `SimpleGAS.Domains.Attribute.Authority` | Attribute events that originated on the server |

## Payload Struct Definitions

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

### FSimpleAbilitySnapshot

```cpp
struct FSimpleAbilitySnapshot
{
    int32 SequenceNumber;             // Used to keep track of the order of snapshots
    FGuid AbilityID;                  // The ID of the ability that took the snapshot
    FGameplayTag SnapshotTag;         // Tag identifying what kind of snapshot this is
    double TimeStamp;                 // When the snapshot was taken (server time)
    FInstancedStruct StateData;       // The data captured in this snapshot
    bool WasClientSnapshotResolved;   // Whether this client snapshot has been resolved against the server
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