---
title: Simple Event Subsystem
layout: home
parent: Concepts
nav_order: 6
---

# Simple Event Subsystem

* [A bundled subsystem](https://github.com/strayTrain/SimpleEventSubsystemPlugin) that allows events to be passed by gameplay tags
    * e.g. You can send an event that a player walked through a door with a payload consisting of a reference to the player. A listener (like a widget, player controller etc) can then react to this event
* Event payloads aren't hardcoded to a type and can be any arbitrary struct
* This is used to send and receive messages from abilities  
    ![screenshot of event setup](../../images/HLO_SimpleEvent.png)
