# Simple Gameplay Ability System Plugin

## Introduction
The Simple Gameplay Ability System plugin provides a streamlined alternative to Unreal Engine's Gameplay Ability System.  

It tries to solve less problems out the box so that you can extend it in ways that suit your game. What problems it does solve are (I hope) your biggest headaches.
Some highlight features:

<details>
	<summary>Can be configured and used with only Blueprints, keeps the config to a minimum. </summary>
</br>
	
| Ability component setup  | Ability setup | Gameplay effect|
| ------------- | ------------- | -------------|
| ![image](https://github.com/user-attachments/assets/7aff04d1-ae6f-4488-a0c7-04d7498eae51)  | ![image](https://github.com/user-attachments/assets/f81744e5-4222-4930-b245-43ed2441ac34)  | ![image](https://github.com/user-attachments/assets/0cd54c36-e3b6-4054-9a00-f2a9f146035e) |

</details>

<details>
	<summary> Easily pass in any struct as context to your abilities with replication support </summary>
  </br>
	When activating the ability  
	
  ![image](https://github.com/user-attachments/assets/4cc3c77e-f4d7-4f93-96e3-79079efa1af8)  
	
  Within the ability
  ![image](https://github.com/user-attachments/assets/710c4058-8ef7-4794-9b4b-ed81ee6fc87f)

</details>

<details>
	</br>
	<summary> A simpler attribute system that supports numerical values or structs for compound data </summary>
	
![image](https://github.com/user-attachments/assets/8ad445d5-c456-4f06-86c7-f331d729b587)
![image](https://github.com/user-attachments/assets/5bdcea57-39dd-4606-aa34-b32e4aab594a)

</details>

<details>
	</br>
	<summary> An extended Simple Event plugin with support for replication </summary>

Read more about [the Simple Event plugin here.](https://github.com/strayTrain/SimpleEventSubsystemPlugin) Simple Gameplay Ability System comes with some built in events like when an attribute changes or an ability ends. A full breakdown of the available events is detailed later.
	
![image](https://github.com/user-attachments/assets/0aa7f42a-939f-4c4c-9a8b-4c0246ea1471)

![image](https://github.com/user-attachments/assets/2db1cecd-b6d1-4842-a0be-56346000d3b1)


</details>

<details>
<summary> An intuitive way of dealing with client predicted abilities in multiplayer scenarios </summary>	
</br>
When using prediction we allow the client to run its own version of the ability as if it had authority while the server also runs the same ability. We define 
structs that keep track of the ability state and when the server and client mismatch, the client corrects their local ability.  
	
</br>
 
![image](https://github.com/user-attachments/assets/baba4c58-c3cb-4be4-b8a9-0e4d8c660ecc)
</details>

## Key Concepts Overview

### 1. `USimpleAbility`
`USimpleAbility` represents a sequence of actions that can involve (but are not limited to) playing animations, spawning particle effects and changing player stats like their mana. Examples of abilities:
- Play a kick animation montage then, at the peak of the kick, check for overlap with enemies, lower their health attribute and launch them into the air.
- Opening and closing a door
- Spawning an area of effect target indicator in the level that the player can control. Upon confirming the location, another ability gets activated that spawns a meteor which crashes into the ground.

Abilities can activate other abilities and additionally support a parent child relationship where: if the parent ability gets cancelled then any child abilities it activated while it was running also get cancelled.  
Abilities also support multiplayer prediction i.e. when a player activates an ability, they don't have to wait for a response from the server before they see feedback on their screen.

### 2. `USimpleAbilityComponent`
`USimpleAbilityComponent` serves as the central component for managing abilities. It provides functions to:
- Grant and revoke abilities to control who can do what.
- Activate abilities.
- Keep track of time between abilities in a multiplayer scenario e.g. When a predicted ability gets activated on the the client and then later on the server, the server version of the ability gets an estimate of how much time passed since the ability was activated on the client.  You can use this to "fast forward" the server version a bit to reduce overall perceived lag in the game.
- Manage attributes such as health, stamina, or custom-defined stats (more on this later)
- Manage gameplay tags, which can be used by abilities to decide on how to activate/behave (e.g. a melee kick ability can't be activated you have the tag "PlayerState.Falling")
- Enable communication between client and server through replicated simple events

This component can be attached to any actor that you want to be able to use abilities. Examples: The player controller, the player pawn or a prop in your game like a door.

### 3. Attributes
Attributes are similar in concept to player stats from RPG games. Think health, strength, stamina etc. 
Attributes are defined with a gameplay tag e.g. "Attributes.Health" and have values associated with them.  
Attributes values come in 2 variants:
1. Float attributes: these attributes have a base value, current value and optional limits on those values. e.g.

![image](https://github.com/user-attachments/assets/b8b536bd-d611-4f48-8b2c-91f5610763a1)

3. Struct attributes: these have a tag, a struct type and a struct value. They can only be set/read in blueprint code and don't support initialization in the inspector.  
![image](https://github.com/user-attachments/assets/3724a265-eddb-49de-8414-81584a67268a)

Every ability component has their own attributes
## Example Setup for Single Player Games
1. **Adding `USimpleAbilityComponent`**: Attach a `USimpleAbilityComponent` to your character blueprint.
2. **Granting Abilities**: Create abilities in Blueprints and grant them to the character using the component's `GrantAbility` function.
3. **Managing Attributes**: Define attributes in a data asset (`UAttributeSet`) and assign them to your character.
4. **Activating Abilities**: Abilities can be activated using the `ActivateAbility` function, which will trigger the appropriate behavior based on conditions and required tags.

## Example Setup for Multiplayer Games
In a multiplayer context, abilities need to synchronize across clients and the server.

1. **Network Authority**: Use the component's built-in functions to handle server and client authority. `USimpleAbility` has several functions that respect different authority policies, such as `ServerInitiated` or `LocalPredicted`.
2. **Granting and Revoking Abilities**: Abilities should be granted on the server to ensure all clients are in sync. Use `GrantAbility` from a server-authority blueprint or C++ script.
3. **Replication**: Attributes and abilities replicate automatically via the plugin. Make sure your gameplay flow respects ownership and authority by calling ability activation from the appropriate client/server context.

## Use Case Examples
> **Note**: Screenshots will be added later to illustrate these examples in action.

1. **Basic Attack Ability**: Create a simple melee attack with a cooldown period.
2. **Movement Buff**: Implement a sprint ability that temporarily increases movement speed.
3. **Environmental Interaction**: Trigger abilities in response to interacting with objects, e.g., picking up items or opening doors.

Each use case can be configured easily using Blueprints and the existing components of the plugin.

## API Reference
This section provides an overview of the main API functions and classes available in the plugin:

### `USimpleAbilityComponent`
- `GrantAbility(TSubclassOf<USimpleAbility> Ability, bool AutoActivate, FInstancedStruct Context)`: Grants an ability to the component.
- `RevokeAbility(TSubclassOf<USimpleAbility> Ability, bool CancelInstances)`: Revokes an ability.
- `ActivateAbility(TSubclassOf<USimpleAbility> AbilityClass, FInstancedStruct Context)`: Activates an ability.
- `SetAvatarActor(AActor* NewAvatarActor)`: Sets the actor controlled by this component.

### `USimpleAbility`
- `InitializeAbility(USimpleAbilityComponent* OwningComponent, FGuid InstanceID, double ActivationTime)`: Initializes an ability instance.
- `Activate(FInstancedStruct Context)`: Activates the ability with the specified context.
- `EndAbility(FGameplayTag EndStatus)`: Ends the ability, providing an end status tag (`Success`, `Cancelled`, etc.).
- `TakeStateSnapshot(FGameplayTag SnapshotTag, FInstancedStruct SnapshotData)`: Captures a state snapshot for predictive gameplay.

### Attributes
- `FFloatAttribute` and `FStructAttribute`: Define and manage attributes using numerical or structured data.
- `SetFloatAttributeValue(EAttributeValueType ValueType, FGameplayTag AttributeTag, float NewValue)`: Sets the value for a float attribute, clamping based on predefined limits.

The plugin provides full flexibility to extend, add custom events, and tailor the abilities and attributes to match your game's requirements.

