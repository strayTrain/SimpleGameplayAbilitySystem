---
title: Attribute Modifiers
layout: home
parent: Blueprint Reference
nav_order: 3
---

# Attribute Modifiers

Attribute Modifiers are the workhorses of SimpleGAS - they're how you actually change attributes during gameplay. Need to deal damage, apply a buff, or add a status effect? Attribute Modifiers are your go-to tool.
Think of Attribute Modifiers as specialized tools that can read, change, and track changes to attributes. They come in two flavors:

- **Instant Modifiers**: Apply their changes immediately and are done (like dealing damage)
- **Duration Modifiers**: Stick around for a while, doing their thing repeatedly (like poison damage over time)

At their core, Attribute Modifiers take an attribute and say "change this value in this way." They can:

- Add or subtract from an attribute
- Multiply an attribute by some value
- Override an attribute with a new value

But they can do much more than just basic math! Modifiers can:

- Chain together multiple attribute changes
- Trigger side effects like visual effects or sounds
- Activate other abilities
- Apply gameplay tags to mark states
- Remove or cancel other modifiers

<details markdown="1">
  <summary>Variables</summary>

| Name        | Type |Description |
|:-------------|:------------------|:------------------|
| OwningAbilityComponent| Actor Reference | A reference to the ability component that owns this ability. This is set when the ability is activated and can be used to access the component's attributes and other functions. |
| CanTick| Bool | a boolean that determines if the ability should tick every frame and call `OnTick`. If set to false, the ability will not tick. |
| Activation Policy| Enum | An enum that controls where the ability can be activated: <br> - `LocalOnly`: Can be activated on client or server, not replicated. Useful for local effects or single player games. <br> - `ClientOnly`: Can be activated only on clients, if the server tries to activate this ability, it will fail. <br> - `ServerOnly`: Can be activated only on the server. The ability will not replicate to clients. <br> - `ClientPredicted`: When activated on the client, the ability will run on the client immediately and then a request to activate it is sent to the server. When activated on the server, the rules of ServerAuthority apply. <br> - `ServerInitiatedFromClient`: Client can request activation, server activates first and then replicates to clients. If called on the server, the rules of ServerAuthority apply. <br> - `ServerAuthority`: Can only be activated on the server but will replicate activation to clients |
| Instancing Policy| Enum | Controls how the ability object is created and managed <br> - `SingleInstance`: Only one instance of the ability object can exist <br> - This is better for performance and memory usage but you need to remember to reset any variables between activations because the same instance is reused. <br> - `MultipleInstances`: A new ability object is created each time the ability is activated |
| Activation Required Tags | Gameplay Tag Container | Tags that must be present on the ability component for the ability to be activated. |
| Activation Blocking Tags | Gameplay Tag Container | Tags that will block the ability from being activated if they are present on the ability component. |
| AbilityTags | Gameplay Tag Container | Categorizes the ability (e.g., "Ability.Attack.Melee") <br> - You can use this to cancel groups abilities with the same tags by calling `CancelAbilitiesWithTags`|
| TemporarilyAppliedTags | Gameplay Tag Container | Tags to add to the activating ability component when this ability is activated and remove when this ability ends (e.g., "PlayerState.Attacking") |
| PermanentlyAppliedTags | Gameplay Tag Container | Tags to add to the activating ability component when this ability is activated. Not automatically removed. |
| Cooldown | Float | A float that determines how long the ability is on cooldown after activation. <br> - If <= 0, the ability can be activated again immediately. |
| Required Context Type | StructType | The context passed to the ability must be of this struct type for the ability to activate. If left empty, any (or no) context type is accepted. |
| Avatar Type Filter | Actor Class Array | The avatar of the owning ability component must be of this type for the ability to activate. If left empty, any (or no) avatar type is accepted. <br> - This is useful for abilities that are only available to certain characters or classes. |
| Require Grant To Activate | Bool | If true, the ability must be granted to the ability component before it can be activated. If false, any ability component can activate this ability. |

</details>

<details markdown="1">
  <summary>Implementable Functions</summary>

<details markdown="1">
  <summary>CanActivate</summary>

Use this to check costs, cooldowns, or other conditions.  

- Inputs: 
  - `ActivationContext`: Context passed to the ability when it was activated
- Outputs: 
  - `bool` (true if the ability can be activated, false otherwise)
- Example:  
  ![a screenshot of the CanActivate function](gameplay_abilities_1.png)

</details>

<details markdown="1">
  <summary>PreActivate</summary>

Called before `OnActivate` if `CanActivate` returns true. Use this for ability setup and modifying resource attributes like Mana/Stamina/Energy etc.  

- Inputs: 
  - `ActivationContext`: Context passed to the ability when it was activated
- Outputs: 
  - none
- Example:  
  ![a screenshot of the PreActivate function](gameplay_abilities_2.png)

</details>

<details markdown="1">
  <summary>OnActivate</summary>

Main execution point for the ability. Remember to call `EndAbility` or `CancelAbility` when the ability is complete/cancelled.

- Inputs: 
  - `ActivationContext`: Context passed to the ability when it was activated
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnActivate function](gameplay_abilities_3.png)

</details>

<details markdown="1">
  <summary>OnTick</summary>

Called every frame while the ability is active if `CanTick` is true. Use this to update the ability's state or perform actions over time.
  - Inputs: 
  - `DeltaTime`: A float representing the time since the last tick
- Outputs: 
  - none

</details>

<details markdown="1">
  <summary>CanCancel</summary>

Determines if the ability can be cancelled when CancelAbility is called. Use this for things like uninterruptible abilities.
- Inputs: 
  - none
- Outputs: 
  - bool (true if the ability can be cancelled, false otherwise)
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_5.png)

</details>

<details markdown="1">
  <summary>OnEnd</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - `EndingStatus`: The gameplay tag passed to the `EndAbility` or `CancelAbility` function
  - `EndingContext`: Context that was passed to the `EndAbility` or `CancelAbility` function
  - `WasCancelled`: Boolean indicating if the ability if `OnEnd` was triggered by `CancelAbility`. If false it was triggered by `EndAbility`.
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

</details>

<details markdown="1">
  <summary>Callable Functions</summary>

<details markdown="1">
  <summary>ActivateSubAbility</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>TakeStateSnapshot</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>GetAvatarActor</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>GetAvatarActorAs</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>ApplyAttributeModifierToTarget</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>IsAbilityActive</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>GetActivationTime</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>GetActivationDelay</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>GetActivationContext</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>WasActivatedOnServer</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>WasActivatedOnClient</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>

<details markdown="1">
  <summary>GetServerRole</summary>

Called when the ability ends (normally or cancelled). Use this to clean up any event listeners or effects that were created while the ability was active.
  - Inputs: 
  - none
- Outputs: 
  - none
- Example:  
  ![a screenshot of the OnEnd function](gameplay_abilities_4.png)

</details>


</details>