---
title: Simple Ability Component
layout: home
parent: Concepts
nav_order: 6
---

# Simple Ability Component

## What Is the Simple Ability Component?

The Simple Ability Component is the core manager for your game's ability system. It ties together abilities, attributes, and gameplay tags, serving as the central hub for gameplay mechanics. This component can be attached to any actor that needs to use abilities or have attributes.

Think of it as the "character sheet" for your actors that:
- Stores and manages gameplay abilities
- Tracks attributes like health, stamina, or custom stats
- Maintains the actor's gameplay tags (states and properties)
- Handles ability activation and networking
- Provides an event system for gameplay communication

![Ability Component Overview](TODO)

<details markdown="1">
  <summary>Class Default Variables</summary>

### Tags
- **GameplayTags**: A container of gameplay tags that represent the actor's current state
  - Used to control ability activation (e.g., "Player.Stunned", "Combat.InProgress")
  - Replicated to clients automatically
  - Can be queried by game systems to determine actor state

### Abilities
- **GrantedAbilities**: Array of ability classes that this component can activate
  - These abilities are granted to the component at startup
  - Can be modified at runtime with GrantAbility/RevokeAbility

- **AbilitySets**: Data assets containing groups of abilities to grant
  - Useful for organizing abilities into logical sets (e.g., "BasicMoves", "FireSpells")
  - All abilities in these sets are granted at startup

- **AbilityOverrideSets**: Data assets for ability class substitutions
  - Lets you replace one ability with another without changing references
  - Useful for character-specific variants of generic abilities

### Attributes
- **FloatAttributes**: Array of numerical attributes (like Health, Mana, Speed)
  - Each attribute has a tag, base value, current value, and optional limits
  - Automatically replicated to clients

- **StructAttributes**: Array of complex structured data attributes
  - For more complex game data (like equipment stats, skill trees)
  - Requires an attribute handler class to process changes

- **AttributeSets**: Data assets containing predefined attributes
  - Similar to AbilitySets but for attributes
  - All attributes in these sets are added at startup

### Avatar
- **AvatarActor**: The actor that this ability component represents
  - Often the owning actor, but can be different (e.g., a controller's avatar is its pawn)
  - Used by abilities to affect the game world
</details>

<details markdown="1">
  <summary>Avatar Actor Functions</summary>

- **GetAvatarActor**: Returns the current avatar actor
  - Use this instead of GetOwner() in abilities

- **SetAvatarActor**: Changes the avatar actor (Authority only)
  - Useful when possessing different pawns

- **IsAvatarActorOfType**: Checks if the avatar is of a specified class
  - Helpful for type-safe ability activation requirements
</details>

<details markdown="1">
  <summary>Ability Functions</summary>

### Ability Management
- **GrantAbility**: Adds an ability to the component (Authority only)
  - Makes the ability available for activation

- **RevokeAbility**: Removes an ability from the component (Authority only)
  - Prevents future activation of the ability

- **AddAbilityOverride**: Substitutes one ability class with another (Authority only)
  - Useful for character variants or upgrades

- **RemoveAbilityOverride**: Removes a previously set override (Authority only)

### Ability Activation
- **ActivateAbility**: Activates an ability with optional context data
  - Primary way to trigger abilities
  - Can include any struct as context data via FInstancedStruct

- **CancelAbility**: Stops a specific ability that's currently running
  - Calls CanCancel() to check if cancellation is allowed

- **CancelAbilitiesWithTags**: Cancels all running abilities with matching tags
  - Useful for cancelling groups of abilities (e.g., all attack abilities)
</details>

<details markdown="1">
  <summary>Attribute Functions</summary>

### Attribute Management
- **AddFloatAttribute**: Adds a numerical attribute to the component
  - Can optionally override an existing attribute with the same tag

- **RemoveFloatAttribute**: Removes a numerical attribute from the component

- **AddStructAttribute**: Adds a complex data attribute to the component
  - Requires a valid attribute handler class

- **RemoveStructAttribute**: Removes a complex data attribute from the component

### Attribute Modifiers
- **ApplyAttributeModifierToTarget**: Applies a modifier to another component's attributes
  - Changes attributes based on the modifier's rules
  - Returns the modifier's unique ID

- **ApplyAttributeModifierToSelf**: Convenience function to apply a modifier to this component
  - Same as ApplyAttributeModifierToTarget but targets self

- **CancelAttributeModifier**: Stops an active attribute modifier
  - For duration modifiers, ends them early
  - For instantaneous modifiers, cancels any ongoing side effects

- **CancelAttributeModifiersWithTags**: Cancels all modifiers with matching tags
  - Useful for removing all effects of a certain type
</details>

<details markdown="1">
  <summary>Tag Functions</summary>

- **AddGameplayTag**: Adds a tag to the component's tag container
  - Optionally broadcasts an event with custom payload data
  - Used to mark states like "Stunned" or "InCombat"

- **RemoveGameplayTag**: Removes a tag from the component's tag container
  - Also broadcasts an event that can carry payload data
  - Used to clear states when they end
</details>

<details markdown="1">
  <summary>Event Functions</summary>

- **SendEvent**: Sends an event through the event system
  - Can include any data via FInstancedStruct payload
  - Supports various replication policies for multiplayer
  - Can target specific listeners or broadcast to all

The component includes built-in networking support for sending events:
  - Local only (no replication)
  - Server and owning client
  - Server and owning client with prediction
  - All connected clients
  - All connected clients with prediction
</details>

<details markdown="1">
  <summary>Utility Functions</summary>

- **HasAuthority**: Checks if this component is on the server or authoritative client

- **GetServerTime**: Returns the current server time
  - Useful for synchronizing ability timings in multiplayer

- **FindAbilityState**: Gets the current state of an ability by ID
  - Returns information about the ability's status and context

- **IsAnyAbilityActive**: Checks if any ability is currently active
  - Useful for conditional logic (e.g., prevent movement while abilities are active)

- **DoesAbilityHaveOverride**: Checks if an ability class has been overridden

- **GetStructAttributeHandler**: Gets a handler for a struct attribute type
  - Handlers process changes to complex attribute data
</details>

<details markdown="1">
  <summary>Tips for Using the Ability Component</summary>

1. **Component Setup**: Add the Ability Component to any actor that needs abilities or attributes

2. **Use Data Assets**: Organize abilities and attributes into AbilitySets and AttributeSets for better management

3. **Avatar Actor**: Always set the avatar actor correctly, especially when the owner isn't the actor performing abilities

4. **Activation Context**: Pass relevant data when activating abilities using FInstancedStruct

5. **Mind Replication**: Be aware of which functions only work on the server (Authority-only functions)

6. **Tag Management**: Use gameplay tags consistently to track states and conditions

7. **Clean Cancellation**: Properly cancel abilities and modifiers when they should end

8. **Event Communication**: Use the SendEvent system instead of direct references between systems

9. **Attribute Organization**: Group related attributes logically and use consistent naming conventions

10. **Blueprint Exposure**: Expose key component functions in your actor blueprints for easier designer access
</details>

The Simple Ability Component provides a flexible foundation for building complex gameplay systems while keeping your code modular and maintainable.