---
title: Simple Ability Component
layout: home
parent: Blueprint Reference
nav_order: 2
---

# Simple Gameplay Ability Component

The Simple Gameplay Ability Component is the core manager for your game's ability system. It ties together abilities, attributes, and gameplay tags, serving as the central hub for gameplay mechanics. This component can be attached to any actor that needs to use abilities or have attributes.

Think of it as the "character sheet" for your actors that:
- Stores and manages gameplay abilities
- Tracks attributes like health, stamina, or custom stats
- Maintains the actor's gameplay tags (states and properties)
- Handles ability activation and networking
- Provides an event system for gameplay communication

## Table of Contents

- [Variables](#variables)
- [Ability Functions](#ability-functions)
- [Tag Functions](#tag-functions)
- [Attribute Functions](#attribute-functions)
- [Event Functions](#event-functions)
- [Utility Functions](#utility-functions)

## Variables

| Name | Type | Description |
|:-----|:-----|:------------|
| Avatar Actor | Actor Reference | The Actor that this ability component controls |
| Granted Abilities | Array of Simple Gameplay Ability Class References | Abilities that are granted to this component |
| Active Ability Overrides | Array of Ability Override | Abilities that override other abilities |
| Attribute Sets | Array of Attribute Set References | Sets containing predefined attributes |
| Ability Override Sets | Array of Ability Override Set References | Sets containing predefined ability overrides |

## Ability Functions

### Activation

| Function | Description | Inputs | Outputs |
|:---------|:------------|:-------|:--------|
| Activate Ability | Activates an ability class | Ability Class, Ability Context, Override Activation Policy (optional), Activation Policy Override (optional) | Success (bool), Ability ID (GUID) |
| Activate Ability With ID | Activates an ability with a specific ID | Ability ID, Ability Class, Ability Context, Override Activation Policy (optional), Activation Policy Override (optional) | Success (bool) |
| Cancel Ability | Cancels a running ability | Ability ID, Cancellation Context (optional) | Success (bool) |
| Cancel Abilities With Tags | Cancels all running abilities that have any of the specified tags | Tags, Cancellation Context (optional) | Cancelled Ability IDs (Array of GUIDs) |
| Is Ability On Cooldown | Checks if an ability is on cooldown | Ability Class | Is On Cooldown (bool) |
| Get Ability Cooldown Time Remaining | Gets the remaining cooldown time for an ability | Ability Class | Time Remaining (float) |
| Is Any Ability Active | Checks if any ability is currently active | None | Is Active (bool) |

### Management

| Function | Description | Inputs | Outputs |
|:---------|:------------|:-------|:--------|
| Grant Ability | Grants an ability to this component | Ability Class | None |
| Revoke Ability | Revokes an ability from this component | Ability Class | None |
| Add Ability Override | Adds an ability override | Original Ability Class, Override Ability Class | None |
| Remove Ability Override | Removes an ability override | Original Ability Class | None |
| Does Ability Have Override | Checks if an ability has an override | Ability Class | Has Override (bool) |
| Is Avatar Actor Of Type | Checks if the avatar actor is of a specific type | Actor Class | Is Of Type (bool) |

## Tag Functions

| Function | Description | Inputs | Outputs |
|:---------|:------------|:-------|:--------|
| Add Gameplay Tag | Adds a gameplay tag to this component | Tag, Payload (optional) | None |
| Remove Gameplay Tag | Removes a gameplay tag from this component | Tag, Payload (optional) | None |
| Has Gameplay Tag | Checks if this component has a specific gameplay tag | Tag | Has Tag (bool) |
| Has All Gameplay Tags | Checks if this component has all specified gameplay tags | Tags | Has All Tags (bool) |
| Has Any Gameplay Tags | Checks if this component has any of the specified gameplay tags | Tags | Has Any Tags (bool) |

## Attribute Functions

### Float Attributes

| Function | Description | Inputs | Outputs |
|:---------|:------------|:-------|:--------|
| Add Float Attribute | Adds a float attribute to this component | Float Attribute, Override If Exists (optional) | None |
| Remove Float Attribute | Removes a float attribute from this component | Attribute Tag | None |
| Has Float Attribute | Checks if this component has a specific float attribute | Attribute Tag | Has Attribute (bool) |
| Get Float Attribute Value | Gets the value of a float attribute | Value Type, Attribute Tag | Was Found (bool), Value (float) |
| Set Float Attribute Value | Sets the value of a float attribute | Value Type, Attribute Tag, New Value | Success (bool), Overflow (float) |
| Increment Float Attribute Value | Increments the value of a float attribute | Value Type, Attribute Tag, Increment | Success (bool), Overflow (float) |
| Override Float Attribute | Completely replaces a float attribute | Attribute Tag, New Attribute | Success (bool) |

### Struct Attributes

| Function | Description | Inputs | Outputs |
|:---------|:------------|:-------|:--------|
| Add Struct Attribute | Adds a struct attribute to this component | Struct Attribute, Override If Exists (optional) | None |
| Remove Struct Attribute | Removes a struct attribute from this component | Attribute Tag | None |
| Has Struct Attribute | Checks if this component has a specific struct attribute | Attribute Tag | Has Attribute (bool) |
| Get Struct Attribute Value | Gets the value of a struct attribute | Attribute Tag | Was Found (bool), Value (Instanced Struct) |
| Set Struct Attribute Value | Sets the value of a struct attribute | Attribute Tag, New Value, Modification Events (optional) | Success (bool) |

### Attribute Modifiers

| Function | Description | Inputs | Outputs |
|:---------|:------------|:-------|:--------|
| Apply Attribute Modifier To Target | Applies an attribute modifier to a target component | Target Component, Modifier Class, Modifier Context | Success (bool), Modifier ID (GUID) |
| Apply Attribute Modifier To Self | Applies an attribute modifier to this component | Modifier Class, Modifier Context | Success (bool), Modifier ID (GUID) |
| Cancel Attribute Modifier | Cancels an attribute modifier | Modifier ID | None |
| Cancel Attribute Modifiers With Tags | Cancels all attribute modifiers with any of the specified tags | Tags | None |

## Event Functions

| Function | Description | Inputs | Outputs |
|:---------|:------------|:-------|:--------|
| Send Event | Sends a gameplay event | Event Tag, Domain Tag, Payload, Sender, Listener Filter (optional), Replication Policy | None |

## Utility Functions

| Function | Description | Inputs | Outputs |
|:---------|:------------|:-------|:--------|
| Get Server Time | Gets the current server time | None | Server Time (double) |
| Get Gameplay Ability Instance | Gets an instance of an ability by ID | Ability ID | Ability Instance |
| Get Attribute Modifier Instance | Gets an instance of an attribute modifier by ID | Attribute ID | Attribute Modifier Instance |

## Replication Modes

The Simple Gameplay Ability Component supports various replication policies for abilities and events:

| Policy | Description |
|:-------|:------------|
| No Replication | The ability or event is not replicated |
| Server And Owning Client | The ability or event is replicated from server to the owning client only |
| Server And Owning Client Predicted | The ability or event runs on client first (prediction) and then is verified by the server |
| All Connected Clients | The ability or event is replicated from server to all connected clients |
| All Connected Clients Predicted | The ability or event runs on client first (prediction) and then is replicated to all clients |

## Example Usage

### Activating an Ability

```
// Create context data for the ability
Variable Type: Instanced Struct
Variable Name: AbilityContext

// Activate the ability
Target: Your Ability Component
Function: Activate Ability
Ability Class: YourAbilityClass
Ability Context: AbilityContext
Success -> Branch
Ability ID -> Store for later reference
```

### Managing Attributes

```
// Add a health attribute
Target: Your Ability Component
Function: Add Float Attribute
Attribute To Add: 
    - Attribute Name: "Health"
    - Attribute Tag: "Attribute.Health"
    - Base Value: 100.0
    - Current Value: 100.0
    - Value Limits:
        - Min Current Value: 0.0
        - Max Current Value: 100.0
        - Use Min Current Value: true
        - Use Max Current Value: true
Override If Exists: true

// Later, apply damage
Target: Your Ability Component
Function: Increment Float Attribute Value
Value Type: Current Value
Attribute Tag: "Attribute.Health"
Increment: -10.0 // Damage amount
```

### Using Tags

```
// Add a tag
Target: Your Ability Component
Function: Add Gameplay Tag
Tag: "PlayerState.Stunned"

// Cancel abilities that shouldn't run while stunned
Target: Your Ability Component
Function: Cancel Abilities With Tags
Tags: ["Ability.Movement", "Ability.Attack"]
```