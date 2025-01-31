# Simple Gameplay Ability System Plugin

A streamlined, flexible, and network‐friendly alternative to Unreal Engine’s Gameplay Ability System. Designed for both single-player and multiplayer projects, this plugin gives you a modular framework for handling abilities, attributes, and events in a way that’s accessible to both Blueprints and C++. You can use this plugin without needing to write any C++ code.

---

## Overview

The **Simple Gameplay Ability System Plugin** provides a lightweight yet powerful framework for creating and managing gameplay abilities. It is built on several core subsystems:

- **Abilities:** Encapsulate game actions (jumping, attacking, opening doors, etc.) with support for custom logic and visual feedback.
- **Ability Component:** A central hub that you attach to an actor (player, enemy, prop, player controller etc.) to manage granted abilities, attribute changes, and event handling.
- **Attributes:** Manage actor stats. Both simple numerical (e.g. health, stamina) and complex structured data stored in structs are supported.
- **Attribute Modifiers:** Apply instantaneous or over-time changes to attributes (such as damage, buffs, or debuffs) in a manner similar to gameplay effects.
- **Event Subsystem:** A flexible, tag-based messaging system that lets different parts of your game communicate without tight coupling.

This system is designed with multiplayer in mind. It supports replication, client prediction, and state reconciliation while remaining accessible for rapid prototyping in Blueprints.

---

## Getting Started

[The docs can be found here](https://straytrain.github.io/SimpleGameplayAbilitySystem/)

## Contributing

Contributions are welcome! To contribute:
1. **Fork the Repository** and create a feature branch.
2. **Update Documentation:**  
   Include any new features or changes.
3. **Submit a Pull Request:**  
   Please provide a clear description of your changes.

I hope this plugin helps you have more fun while making your game!  
If you have any questions, suggestions, or encounter issues, please open an issue on GitHub or reach out via the repository’s contact options.
