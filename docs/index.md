---
title: Introduction
layout: home
nav_order: 1
---

# SimpleGAS: A More Approachable Gameplay Ability System

**SimpleGAS** is an alternative to Epic's Gameplay Ability System that focuses on developer experience. It's designed to be easier to understand while still giving you the tools to create complex gameplay mechanics.

## What's SimpleGAS About?

Epic's Gameplay Ability System (GAS) is powerful but comes with a steep learning curve.  
SimpleGAS aims to provide much of the same functionality with:

- Full Blueprint support
- Less boilerplate code
- Straightforward documentation with examples

Check out the [examples](pages/examples.html) to see if SimpleGAS fits your project's needs.

## Features

### Blueprint-Friendly Design

Create abilities without writing C++ code:

- SimpleGAS helps you implement mechanics like:
  - Character abilities with cooldowns
  - Status effects and buffs/debuffs
  - Stat based damage systems
  - Ability combos and interactions

### A Modular Ability System

Break down your gameplay mechanics into reusable pieces:

- **SimpleAbilities**: Self-contained gameplay actions that can activate sub abilities

### Flexible Attribute System

Manage gameplay stats like health or energy. Supports floats and structs.

- **Float Attributes**: Numerical values like health or mana
- **Struct Attributes**: Complex data structures
- **Attribute Modifiers**: Apply temporary or permanent changes to attributes

### Multiplayer Support

SimpleGAS includes tools for multiplayer games:

- Client prediction for responsive gameplay
- Server authority for consistency
- Automatic state correction with optional manual intervention

### Event Based Communication

Communicate between game systems using gameplay tags:
- Use hierarchical tags to organize events
- Send and receive custom data
- Filter events with specific criteria
- Connect different systems together without hard dependencies between them.