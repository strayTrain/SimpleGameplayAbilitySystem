---
title: Simple Ability Component
layout: home
parent: Blueprint Reference
nav_order: 2
---

# Simple Gameplay Ability Component
{: .no_toc }

<details close markdown="block">
<summary> Table of contents </summary>
{: .no_toc .text-delta }

1. TOC
{:toc}
</details>

## Overview
{: .no_toc }

The Simple Gameplay Ability Component is the core manager for your game's ability system. It ties together abilities, attributes, and gameplay tags, serving as the central hub for gameplay mechanics. This component can be attached to any actor that needs to use abilities or have attributes.

Think of it as the "character sheet" for your actors that:
- Stores and manages gameplay abilities
- Tracks attributes like health, stamina, or custom stats
- Maintains the actor's gameplay tags (states and properties)
- Handles ability activation and networking
- Provides an event system for gameplay communication

<div class="api-docs" markdown="1">

## Properties

| Name | Type | Description |
|:-----|:-----|:------------|
| Avatar Actor | Actor Reference | The Actor that this ability component controls |
| Ability Sets | Array of Ability Set References | Sets containing predefined abilities to grant |
| Ability Override Sets | Array of Ability Override Set References | Sets containing predefined ability overrides |
| Granted Abilities | Array of Simple Gameplay Ability Class References | Abilities that are granted to this component |
| Active Ability Overrides | Array of Ability Override | Abilities that override other abilities |
| Attribute Sets | Array of Attribute Set References | Sets containing predefined attributes |
| Float Attributes | Array of Float Attributes | Float attributes to initialize on this component |
| Struct Attributes | Array of Struct Attributes | Struct attributes to initialize on this component |

## Avatar Actor Functions

### GetAvatarActor

Returns the avatar actor that this ability component is controlling. The avatar is the actor that performs abilities.

**Parameters:**

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | Actor | The avatar actor or null if none has been set |

### SetAvatarActor

Sets the avatar actor for this ability component. This function is available only on the server.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| New Avatar Actor | Actor | The actor to set as the avatar |

### IsAvatarActorOfType

Checks if the avatar actor is of a specific type. Useful for ensuring abilities only activate with appropriate avatar types.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Avatar Class | TSubclassOf&lt;Actor&gt; | The class to check against |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if the avatar actor is of the specified type, false otherwise |

## Ability Functions

### GrantAbility

Grants an ability to this component, allowing it to be activated. This function is available only on the server.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Ability Class | TSubclassOf&lt;USimpleGameplayAbility&gt; | The class of the ability to grant |

### RevokeAbility

Revokes a previously granted ability from this component. This function is available only on the server.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Ability Class | TSubclassOf&lt;USimpleGameplayAbility&gt; | The class of the ability to revoke |

### AddAbilityOverride

Defines an ability override, which replaces one ability with another. Useful for equipment or status effects that modify abilities. This function is available only on the server.
The way an ability override works is that when you activate an ability, the system checks if there is an override for it. If there is, it uses the override instead of the original ability. This allows you to change the behavior of abilities without modifying their original code.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Ability | TSubclassOf&lt;USimpleGameplayAbility&gt; | The original ability class to be overridden |
| Override Ability | TSubclassOf&lt;USimpleGameplayAbility&gt; | The ability class that will replace the original |

### RemoveAbilityOverride

Removes a previously defined ability override. This function is available only on the server.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Ability | TSubclassOf&lt;USimpleGameplayAbility&gt; | The original ability class whose override should be removed |

### DoesAbilityHaveOverride

Checks if an ability has an override set for it on this ability component.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Ability Class | TSubclassOf&lt;USimpleGameplayAbility&gt; | The class of the ability to check |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if the ability has an override, false otherwise |

### ActivateAbility

Activates an ability on this component. This is the main function for initiating abilities.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Ability Class | TSubclassOf&lt;USimpleGameplayAbility&gt; | The class of the ability to activate |
| Ability Context | FInstancedStruct | Context data for the ability activation |
| Override Activation Policy | bool | Whether to override the ability's default activation policy |
| Activation Policy Override | EAbilityActivationPolicy | The activation policy to use if overriding: <br> - `LocalOnly`: Activates on client or server but doesn't replicate (best for single-player or cosmetic effects). <br> - `ClientOnly`: Only activates on clients (for client-side effects). <br> - `ServerOnly`: Only activates on server without replicating to clients. <br> - `ClientPredicted`: Client activates immediately then sends request to server; supports state snapshots and prediction. <br> - `ServerInitiatedFromClient`: Client requests activation, server runs first, then replicates to client. <br> - `ServerAuthority`: Only activates on server but replicates to clients. |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Was Activated | bool | Whether the ability was successfully activated |
| Ability ID | FGuid | The unique ID of the activated ability instance |

### CancelAbility

Cancels a running ability. The ability will only be cancelled if it allows cancellation through its CanCancel function.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Ability Instance ID | FGuid | The ID of the ability instance to cancel |
| Cancellation Context | FInstancedStruct | Optional context data for the cancellation |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if the ability was successfully cancelled |

### CancelAbilitiesWithTags

Cancels all running abilities that have any of the specified tags. Useful for cancelling groups of related abilities.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Tags | FGameplayTagContainer | The tags to check against |
| Cancellation Context | FInstancedStruct | Optional context data for the cancellation |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | TArray&lt;FGuid&gt; | Array of ability IDs that were cancelled |

### IsAbilityOnCooldown

Checks if an ability is currently on cooldown and cannot be activated.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Ability Class | TSubclassOf&lt;USimpleGameplayAbility&gt; | The class of the ability to check |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if the ability is on cooldown, false otherwise |

### GetAbilityCooldownTimeRemaining

Gets the remaining cooldown time for an ability in seconds.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Ability Class | TSubclassOf&lt;USimpleGameplayAbility&gt; | The class of the ability to check |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | float | Remaining cooldown time in seconds (0 if not on cooldown) |

### IsAnyAbilityActive

Checks if any ability is currently active on this component.

**Parameters:**

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if any ability is active, false otherwise |

## Attribute Functions

### AddFloatAttribute

Adds a float attribute to this component. This function is available only on the server.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Attribute To Add | FFloatAttribute | The attribute to add with all its properties |
| Override Values If Exists | bool | If true and the attribute already exists, replace its values |

### RemoveFloatAttribute

Removes a float attribute from this component. This function is available only on the server.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Attribute Tag | FGameplayTag | The tag of the attribute to remove |

### AddStructAttribute

Adds a struct attribute to this component. This function is available only on the server.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Attribute To Add | FStructAttribute | The attribute to add with all its properties |
| Override Values If Exists | bool | If true and the attribute already exists, replace its values |

### RemoveStructAttribute

Removes a struct attribute from this component. This function is available only on the server.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Attribute Tag | FGameplayTag | The tag of the attribute to remove |

### HasFloatAttribute

Checks if this component has a specific float attribute.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Attribute Tag | FGameplayTag | The tag of the attribute to check for |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if the attribute exists, false otherwise |

### HasStructAttribute

Checks if this component has a specific struct attribute.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Attribute Tag | FGameplayTag | The tag of the attribute to check for |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if the attribute exists, false otherwise |

### GetFloatAttributeValue

Gets the value of a float attribute. Float attributes have multiple value types including base value, current value, min/max values, etc.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Value Type | EAttributeValueType | Which value to get: <br> - `CurrentValue`: The actual gameplay value used in calculations <br> - `BaseValue`: The base/permanent value without temporary modifications <br> - `MaxCurrentValue`: The maximum limit for CurrentValue <br> - `MinCurrentValue`: The minimum limit for CurrentValue <br> - `MaxBaseValue`: The maximum limit for BaseValue <br> - `MinBaseValue`: The minimum limit for BaseValue |
| Attribute Tag | FGameplayTag | The tag of the attribute to get |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Was Found | bool | Whether the attribute was found |
| Return Value | float | The value of the attribute |

### SetFloatAttributeValue

Sets the value of a float attribute. Handles clamping values to respect min/max limits.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Value Type | EAttributeValueType | Which value to set: <br> - `CurrentValue`: The actual gameplay value used in calculations <br> - `BaseValue`: The base/permanent value without temporary modifications <br> - `MaxCurrentValue`: The maximum limit for CurrentValue <br> - `MinCurrentValue`: The minimum limit for CurrentValue <br> - `MaxBaseValue`: The maximum limit for BaseValue <br> - `MinBaseValue`: The minimum limit for BaseValue |
| Attribute Tag | FGameplayTag | The tag of the attribute to set |
| New Value | float | The new value to set |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | Whether the operation was successful |
| Overflow | float | Any excess value that couldn't be applied due to clamping |

### OverrideFloatAttribute

Completely replaces a float attribute with a new one. This function is available only on the server.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Attribute Tag | FGameplayTag | The tag of the attribute to override |
| New Attribute | FFloatAttribute | The new attribute to replace it with |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | Whether the operation was successful |

### GetStructAttributeValue

Gets the value of a struct attribute.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Attribute Tag | FGameplayTag | The tag of the attribute to get |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Was Found | bool | Whether the attribute was found |
| Return Value | FInstancedStruct | The value of the attribute |

### SetStructAttributeValue

Sets the value of a struct attribute.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Attribute Tag | FGameplayTag | The tag of the attribute to set |
| New Value | FInstancedStruct | The new value to set |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | Whether the operation was successful |

## Attribute Modifier Functions

### ApplyAttributeModifierToTarget

Applies an attribute modifier to a target component. Attribute modifiers can change attributes with more complex logic than direct setting.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Modifier Target | USimpleGameplayAbilityComponent* | The component to apply the modifier to |
| Modifier Class | TSubclassOf&lt;USimpleAttributeModifier&gt; | The modifier class to apply |
| Modifier Context | FInstancedStruct | Context data for the modifier |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | Whether the modifier was successfully applied |
| Modifier ID | FGuid | The unique ID of the applied modifier |

### ApplyAttributeModifierToSelf

Applies an attribute modifier to this component. Convenience wrapper for applying a modifier to self.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Modifier Class | TSubclassOf&lt;USimpleAttributeModifier&gt; | The modifier class to apply |
| Modifier Context | FInstancedStruct | Context data for the modifier |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | Whether the modifier was successfully applied |
| Modifier ID | FGuid | The unique ID of the applied modifier |

### CancelAttributeModifier

Cancels an active attribute modifier. If the modifier is a duration type, it will end prematurely.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Modifier ID | FGuid | The ID of the modifier to cancel |

### CancelAttributeModifiersWithTags

Cancels all attribute modifiers that have any of the specified tags. Useful for removing groups of related effects.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Tags | FGameplayTagContainer | The tags to check against |

## Gameplay Tag Functions

### AddGameplayTag

Adds a gameplay tag to this component. If the tag is already present, its counter is incremented.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Tag | FGameplayTag | The tag to add |
| Payload | FInstancedStruct | Optional payload data to send with the tag added event |

### RemoveGameplayTag

Removes a gameplay tag from this component. 
Gameplay tags use reference counting, so if you add the same tag multiple times, it will only be removed when all references are removed.

e.g. You have two abilities (A and B) that temporarily add the same gameplay tag and we activate both of them at the same time. If ability A ends before ability B it will try to remove the temporary tag that it added. But since ability B is still active, the tag will not be removed, instead its reference counter will be decremented. The tag will only be removed when ability B ends and the tag reference counter is 0.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Tag | FGameplayTag | The tag to remove |
| Payload | FInstancedStruct | Optional payload data to send with the tag removed event |

### HasGameplayTag

Checks if this component has a specific gameplay tag.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Tag | FGameplayTag | The tag to check for |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if the component has the tag |

### HasAllGameplayTags

Checks if this component has all of the specified gameplay tags.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Tags | FGameplayTagContainer | The tags to check for |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if the component has all the tags |

### HasAnyGameplayTags

Checks if this component has any of the specified gameplay tags.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Tags | FGameplayTagContainer | The tags to check for |

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if the component has any of the tags |

## Event Functions

### SendEvent

Sends a gameplay event from this component. Events can be used for communication between game systems.

**Parameters:**

| Input | Type | Description |
|:-------------|:------------------|:------|
| Event Tag | FGameplayTag | The main tag identifying the event |
| Domain Tag | FGameplayTag | Secondary tag qualifying the event domain |
| Payload | FInstancedStruct | Structured data for the event |
| Sender | AActor* | The actor that is sending the event |
| Listener Filter | TArray&lt;UObject*&gt; | Only send the event to these listeners |
| Replication Policy | ESimpleEventReplicationPolicy | Controls how the event is replicated: <br> - `NoReplication`: Event is only sent locally <br> - `ServerAndOwningClient`: Event is sent from server to owning client <br> - `ServerAndOwningClientPredicted`: Event runs on client first, then is verified by server <br> - `AllConnectedClients`: Event is sent from server to all clients <br> - `AllConnectedClientsPredicted`: Event runs on client first, then is sent to all clients |

## Utility Functions

### GetServerTime

Returns the current server time. On the server this is the actual server time, on clients it's their estimation of the server time. You can override this function in child classes to provide a time implementation that suits your needs.

**Parameters:**

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | double | The current server time in seconds |

### HasAuthority

Checks if this component has network authority (is on the server).

**Parameters:**

| Output | Type | Description |
|:-------------|:------------------|:------|
| Return Value | bool | True if this component has authority |

</div>