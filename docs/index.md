---
title: Introduction
layout: home
nav_order: 1
---

# Simple Gameplay Ability System (SimpleGAS)

[SimpleGAS](https://github.com/strayTrain/SimpleGameplayAbilitySystem) is a streamlined alternative to Epic's [**Gameplay Ability System (GAS)**](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine) with a focus on developer experience and ease of understanding.

## Why Use SimpleGAS?

- **GAS** is powerful but comes with a steep learning curve, has a lot of boilerplate code, and requires some knowledge of C++ to use.  
- **SimpleGAS** is designed to be more accessible, allowing for purely blueprint-based setup and operation. While it has fewer built-in features than **GAS**, it is significantly easier to use, understand, and extend.

## Core Features

### Create Modular and Reusable Gameplay Mechanics

SimpleGAS allows you to break down your gameplay mechanics into **SimpleAbilities**: self-contained gameplay actions that can be activated with an input payload.

- Abilities can be reused and combined to create complex interactions.
- Any struct can be passed as an input payload, allowing for flexible customization.

Example: Launching a player using a custom struct called `LaunchParams`.

![Ability activation with input parameters](images/index_ability_activation_example.png)

Inside the ability:
`
![Launch player ability](images/index_launch_player_example.png)

### Comes With a Flexible Attribute System

`SimpleGameplayAbilityComponent` provides an **Attribute** system to manage gameplay stats like health, stamina, or velocity.

- **Float Attributes**: Represent simple numerical values.
- **Struct Attributes**: Use arbitrary structs for more complex stats.
- **Automatic Events**: When attributes change, events are sent automatically, making it easy to react to changes (e.g., updating UI elements).
- **Attribute Sets**: Attributes can be grouped into reusable data assets.

Example: Setting up a float attribute like Health:

![Float Attribute Example](images/index_float_attribute_example.png)

### Dynamic Attribute Modifiers

Attribute modifiers allow you to create effects like buffs, debuffs, and damage over time effects.

- **Modify multiple attributes at once**.
- **Instant or over-time application**.
- **Trigger side effects** such as activating abilities or applying additional status effects.

Example Use Cases:

- A buff that permanently increases a player's health and damage.
- Damage calculation based on armor:
  - If the target has an `Armor` attribute, reduce that first before affecting `Health`.
  - If they don’t, apply full damage to `Health`.
- A **Burning** status effect:
  - Deals initial damage and periodic damage over 5 seconds.
  - Adds fire particles while active.
  - Explodes on removal, spreading to nearby targets.

### Multiplayer-Ready with Predictive Syncing

SimpleGAS is designed with multiplayer in mind, providing built-in support for:

- **Replication**: Abilities, attributes, and modifiers sync across clients and the server.
- **Client Prediction**:
  - Immediate feedback for smooth gameplay, even with high latency.
  - Abilities take **State Snapshots** that store gameplay state, allowing for seamless correction of client-server differences.
- **Automated Attribute Correction**:
  - Clients apply modifiers instantly.
  - If the server determines the modifier was invalid, the client automatically corrects its state.

Example Workflow:

1. A client predicts a hit, reducing the target’s health and triggering a hit reaction ability.
2. The server decides the hit was actually blocked.
3. The client reverts the incorrect health change and cancels the hit reaction ability.
