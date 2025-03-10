---
title: Simple Ability
layout: home
parent: Blueprint Reference
nav_order: 1
---

# Simple Gameplay Abilities

## What Are Simple Gameplay Abilities?

Simple Gameplay Abilities are self-contained blocks of gameplay logic that define discrete actions or behaviors in your game. They're the building blocks of your gameplay systems - anything from basic attacks and spells to movement abilities and status effects.

Think of abilities as modular "gameplay scripts" that:
- Can be granted to and activated by ability components
- Have their own lifecycle (activation, execution, ending)
- Can modify attributes and apply effects
- Can be networked and predicted for multiplayer games

![Simple gameplay ability example](TODO)

<details markdown="1">
  <summary>Class Default Variables</summary>

When creating a Simple Gameplay Ability, you'll configure these key variables:

### Activation Settings

- **Activation Policy**: Controls how the ability is replicated in multiplayer games
  - **Local Only**: Runs only on the local machine, no replication
  - **Client Only**: Runs only on clients, not on the server
  - **Server Only**: Runs only on the server with no client visibility
  - **Client Predicted**: Runs immediately on the client, then validated by the server
  - **Server Initiated**: Client requests activation, but server activates first
  - **Server Authority**: Server-only with replication to clients

- **Instancing Policy**: Controls how many instances can exist at once
  - **Single Instance**: Only one instance can be active at a time
  - **Multiple Instances**: Multiple activations create separate instances

### Tags

- **Ability Tags**: Categorize the ability (e.g., "Ability.Attack.Melee")
- **Temporarily Applied Tags**: Added during activation and removed when the ability ends (e.g., "PlayerState.Attacking")
- **Permanently Applied Tags**: Added during activation but not automatically removed

### Activation Requirements

- **Activation Required Tags**: Tags that must be present for activation (e.g., "Player.Grounded")
- **Activation Blocking Tags**: Tags that prevent activation (e.g., "Player.Stunned")
- **Required Context Type**: Optional struct type required in activation context
- **Avatar Type Filter**: Restricts the ability to specific actor types
- **Require Grant To Activate**: If true, ability must be explicitly granted before activation
</details>

<details markdown="1">
  <summary>Overridable Functions</summary>

These functions can be overridden in Blueprint to customize your ability's behavior:

### Core Lifecycle Functions

- **CanActivate**: Determines if the ability can be activated
  - Return `true` to allow activation, `false` to block it
  - Check costs, cooldowns, or other conditions here

- **PreActivate**: Called immediately before activation if CanActivate returns true
  - Use for setup work, consuming resources, etc.
  - Called before OnActivate

- **OnActivate**: Main execution point for the ability
  - Implement your core ability logic here
  - You must call EndAbility manually when finished

- **OnEnd**: Called when the ability ends (normally or cancelled)
  - Clean up any ongoing effects
  - Parameters tell you how the ability ended

- **CanCancel**: Determines if the ability can be cancelled
  - Return `true` to allow cancellation, `false` to prevent it

### State Resolution Functions

- **ClientResolvePastState**: Handles correction when client prediction differs from server state
  - Resolve any differences between predicted and authoritative state
</details>

<details markdown="1">
  <summary>Non-Overridable Functions</summary>

These built-in functions can be called from your ability logic:

### Ability Control

- **EndAbility**: Properly ends the ability
  - `EndStatus`: Tag describing how it ended
  - `EndingContext`: Optional data about how it ended

- **CancelAbility**: Cancels the ability prematurely
  - Similar to EndAbility but marks as cancelled

- **ActivateSubAbility**: Activates another ability as part of this one
  - Creates a child ability that can be tracked by the parent
  - Returns the new ability's ID

### State Management

- **TakeStateSnapshot**: Records the current state for networking/prediction
  - Useful for recording key points in the ability's execution
  - Used for resolving client/server differences

### Utility Functions

- **GetAvatarActorAs**: Gets the avatar actor cast to a specific type
- **ApplyAttributeModifierToTarget**: Applies a modifier to a target's attributes
- **IsAbilityActive**: Checks if the ability is currently running
- **GetActivationTime**: Returns when the ability was activated
- **GetActivationDelay**: Time difference between activation and now
- **GetActivationContext**: Returns the original activation context
- **WasActivatedOnServer**: Checks if this ability was server-activated
- **WasActivatedOnClient**: Checks if this ability was client-activated
</details>

<details markdown="1">
  <summary>Utility Nodes</summary>

SimpleGAS provides several utility nodes specifically for use with abilities:

### Async Action Nodes

- **WaitForFloatAttributeChange**: Waits for a specific attribute to change
  - Great for reacting to health/mana changes

- **WaitForAbility**: Waits for another ability to end
  - Useful for chaining abilities together sequentially

- **WaitForSimpleEvent**: Waits for a specific event to be triggered
  - Can wait for any event sent through the event system

### Attribute Modifiers

- Apply attribute modifiers through the ability
- Chain modifiers together for complex effects

### Tag Management

- Add or remove tags during ability execution
- Check for tags on the ability component
</details>

<details markdown="1">
  <summary>Tips for Creating Effective Abilities</summary>

1. **End Your Abilities**: Always call `EndAbility()` when your ability logic is complete
2. **Use Sub-Abilities**: Break complex abilities into smaller, reusable pieces
3. **Handle Prediction**: Use state snapshots for client-predicted abilities
4. **Prefer Tags for Conditions**: Use gameplay tags instead of hard-coded logic for activation conditions
5. **Clean Up Effects**: Make sure temporary effects are properly removed in `OnEnd`
6. **Mind the Network**: Consider how your ability will behave in multiplayer
7. **Document Context Types**: Clearly document what data your ability expects in its activation context
8. **Design for Reusability**: Create abilities that can be reused and combined in different ways
</details>

By thoughtfully designing your abilities, you can build a flexible gameplay foundation that's easy to expand and maintain.