# Simple Gameplay Ability System Plugin

## Introduction
The Simple Gameplay Ability System plugin provides a streamlined alternative to Unreal Engine's [Gameplay Ability System](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine).  

It tries to solve less problems out the box so that you can extend it in ways that suit your game. What problems it does solve are (I hope) your biggest headaches.  

Some highlight features:

<details>
	<summary>Can be configured and used purely in Blueprints, keeps the config to a minimum. </summary>
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
![image](https://github.com/user-attachments/assets/c61c105d-5ed4-43d4-b378-d5b7e9af4db4)


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

Abilities also support multiplayer prediction i.e. when a player activates an ability, they don't have to wait for permission from the server before they see feedback on their screen.

### 2. `USimpleAbilityComponent`
`USimpleAbilityComponent` serves as the central component for managing abilities. It provides functions to:
- Grant and revoke abilities to control who can do what.
- Activate abilities.
- Keep track of time between abilities in a multiplayer scenario e.g. When a predicted ability gets activated on the the client and then later on the server, the server version of the ability gets an estimate of how much time passed since the ability was activated on the client.
- Manage attributes such as health, stamina, or compound attributes like a Vector representing movement direction (more on this in the next section)
- Manage gameplay tags, which can be used by abilities to decide on how to activate/behave (e.g. a melee kick ability can't be activated you have the tag "PlayerState.Falling")
- Enable communication between client and server through replicated simple events

This component can be attached to any actor that you want to be able to use abilities. Examples: The player controller, the player pawn or a prop in your game like a door.

### 3. Attributes
Attributes are similar in concept to player stats from RPG games. Think health, strength, stamina etc.  
Every ability component has their own attributes and the attributes are replicated. When attributes get changed/added/removed, events are automatically sent so that you can react to those changes in your widgets, abilities, etc.  
Attributes are identified with a gameplay tag (e.g. "Attributes.Health") and have values associated with them. They also have an `AttributeName` string which is used to make them more readable in the inspector.
Attributes come in 2 variants:
1. Float attributes: these attributes have a base value, current value and optional limits on those values. Use these to represent simple stats like health. e.g.  
  ![image](https://github.com/user-attachments/assets/b8b536bd-d611-4f48-8b2c-91f5610763a1)

2. Struct attributes: these have an `AttributeTag`, an `AttributeType` and an `AttributeValue`. They can only be set/read in blueprint code.
   You get the benefits of replication and automatically sent events for values that aren't strictly single numbers. e.g.  
  ![image](https://github.com/user-attachments/assets/3724a265-eddb-49de-8414-81584a67268a)

Under the hood, struct attributes are really [FInstancedStruct's](https://forums.unrealengine.com/t/can-someone-show-my-how-to-use-finstancedstruct-please/1898788?utm_source=chatgpt.com). The reason the struct attribute has an `AttributeType` is to make sure when we give it an instanced struct, the underlying struct it holds matches the type we expect.

### 4. Attribute Modifiers
Attribute modifiers behave similarly to gameplay effects from the Gameplay Ability System.
They are a bundle of multiple attribute changes that can be applied to an Ability component. Additionally they can modify tags on the ability component, cancel running abilities and cancel other attribute modifiers.
Attribute modifiers can be applied over time e.g. when the player gets set on fire we apply a Burning attribute modifier which lasts 3 seconds and deals 1 damage to health per second.  


Here's an example of a modifier that reduces the player `Armour` attribute's current value by 15. Let's say the armour attribute was defined with a minimum current value of 0:  
If the armour reduction goes below the minimum current value the remainder is called the `overflow` and is passed on to the next modifier in the stack to use.
In this example, we reduce armour and then subtract any overflow from health.  

![image](https://github.com/user-attachments/assets/449f5c55-70c1-4a2b-995d-03cbeb64e94a)

There are 2 types of attribute modifiers:
1. `Instant`: These modifiers apply their changes and are then over. Examples of instant modifiers are: dealing damage, restoring health
2. `Duration`: These modifiers stick around for a certain amount of time and apply a bundle of attribute modifications at regular intervals e.g. burn damage every second for 3 seconds.

   `Duration` modifiers can also have an infinite duration and no tick interval (i.e this modifier sticks around until we manually remove it or it gets removed by another modifier. With no tick interval, only a single attribute stack change gets applied).
   This is useful for buff/debuff type effects e.g. A movement slowing modifier which adds a tag "PlayerState.Slowed" and decreases the players MovementSpeed attribute by 10. Later, when the modifier is removed the players MovementSpeed attribute is increased by 10)

## Key concepts by example:

Let's make some abilities to illustrate how eveything ties together.  
We'll start with the minimum single player setup and work our way up to a more complicated client predicted multiplayer ability.  
For these examples I'll be using the third person template project.

<details>
	<summary>Installing the plugin</summary>
This plugin requires Unreal Engine 5.2 and later to work **(Note from the dev: I still need to test this with 5.5, where `FInstancedStruct` moved from a plugin to being a part of the engine)**

* Download or clone this repository into your Unreal Engine project under your project's Plugins folder, create the Plugins folder if it doesn't exist. (e.g. If your project folder is `C:\Projects\SimpleGASTest` then place SimpleGameplayAvilitySystem in `C:\Projects\SimpleGASTest\Plugins`)  
* Rebuild your project.  
* Enable the plugin in your Unreal Engine project by navigating to Edit > Plugins and searching for "SimpleGameplayAbilitySystem". (it should be enabled by default)
</details>


<details>
<summary>A minimal jump ability</summary>
	
1. Add a SimpleAbilityComponent to the third person pawn  
  ![image](https://github.com/user-attachments/assets/d61498dc-e09b-43e0-a134-5a7e12d73ae4)
2. Create a new SimpleAbility blueprint class and call it GA_Jump. Once you create it take a look at the default variables.  
  For now we only need to give the ability a name tag. I used `Abilities.Jump`  
  ![image](https://github.com/user-attachments/assets/da63f9f8-4ea2-44e3-b842-8a80c001d5ac)
3. Next we add this newly created ability to the list of abilities that our `AbilityComponent` can activate. Select the `AbilityComponent` we added to the player and add an entry under Gameplay Abilities > Granted Abilities
   ![image](https://github.com/user-attachments/assets/6f3fa22e-6aec-4e09-b996-34228109ebb7)
4. In order to use the ability we need to set an `AvatarActor` for the `AbilityComponent`. An `AvatarActor` is the actor who will "execute" the ability. In our case the avatar is the player pawn.
   ![image](https://github.com/user-attachments/assets/6e6f9158-97b8-49ff-a5f8-4f3f2a30cf5d)
5. To activate the ability we simply call:  
  ![image](https://github.com/user-attachments/assets/d66667ad-f963-433b-82cf-40e8d4ee7720)
6. If you try and activate the ability, you'll notice nothing happens. This is because we still need to add logic to GA_Jump. In your GA_Jump ability override the `OnActivate` event like so:
   ![image](https://github.com/user-attachments/assets/b9e6c278-8358-4d25-a90d-013cf8ea5db7)  
  There are 2 nodes of interest here.  
  * `GetAvatarActorAs` is a utility node that casts the avatar actor (which is an `AActor`) to your subclass (in my case a blueprint child called `BP_Player`
  * `EndAbilitySuccess` tells the ability that it has finished running.

If you run the game and press G you'll see that your player will jump.
</details>

TODO: Add examples for: 
* Launch in a direction passed through the context, show how we can use gameplay tags to control when abilities get activated
* A self damaging ability to explain attribute modification
* Getting a widget to listen to health changes and show them on screen
* A kick ability which plays a montage i.e. Is not over instantly like the jump example. Use to explain `InstancingPolicy`
* Kick + waiting for events + directly changing the health attribute of a hit player
* Previous example but using an AttributeModifier instead
* Build on the kick example introducing multiplayer concepts like `ActivationPolicy`
* Build on the kick example introducing `StateSnapshot` and `StateResolver`
* An example of extending the SimpleAbilityComponent class. Also explain `ServerTime`, how it is synced and how to add your own implementation 



## API Reference
TODO: Collect screenshots and add code samples

