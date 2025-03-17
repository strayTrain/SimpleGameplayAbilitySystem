---
title: Introduction
layout: home
nav_order: 1
has_toc: false
---

# SimpleGAS: A More Approachable Gameplay Ability System

**SimpleGAS** is an alternative to Epic's Gameplay Ability System that focuses on developer experience. It's designed to be easier to understand while still giving you the tools to create complex gameplay mechanics.

## What problems does this plugin solve?

Imagine you're building a multiplayer game where characters can do special moves - maybe casting fireballs, teleporting, or healing teammates. Each of these abilities needs:

- A way to activate (button press, etc.)
- Rules for when it can be used (cooldowns, resource costs)
- Visual effects and animations
- Network synchronization

Without a system to orchestrate all this, you'd end up writing custom code for each ability leading to messy, duplicated logic scattered throughout your project.

[Epic's Gameplay Ability System (GAS)](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine) provides a powerful framework for all these tasks but comes with a steep learning curve and a requirement that you can code a little bit of C++.    
SimpleGAS aims to provide much of the same functionality as Epic's GAS but with:

- Full Blueprint support
- A smaller API with plenty of hooks to extend it
- A focus on modularity and reusability

Check out the [examples](pages/examples/examples.html) to see if SimpleGAS fits your project's needs.