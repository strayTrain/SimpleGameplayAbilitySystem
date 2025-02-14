---
title: Replication and Prediction
layout: home
parent: Concepts
nav_order: 7
---

# Replication and Prediction in SimpleGAS

Gameplay Abilities in SimpleGAS can be replicated and client predicted. Here's how it works:

The lifecycle of an ability is managed by the **AbilityComponent** through a struct called **AbilityState**.

An **AbilityState** is a struct that contains:
* Information about the ability that was activated (it's class, unique identifier, context etc)
* A status enum representing if the ability is active, ended or cancelled
* An array of **StateSnapshots** representing the state of the ability at different times

The **AbilityComponent** maintains a 2 arrays of **AbilityState**: 
* An authoritative replicated array only written to by the server
* A predicted non replicated array used by clients when activating predicted abilities  

When we activate an ability we create an **AbilityState** for it and add it to the appropriate state array (authoritative for the server and predicted for the client)  

### When an ability is activated on the server:
1. An instance of the ability is created on the server and a matching state is added to the authoritative **AbilityState** list
2. The authoritative state is replicated to all clients
3. Upon receiving the new state, the client activates the ability with the same unique identifier and context it received from the authoritative state 

### When an predicted ability is activated on the client:
1. An instance of the ability is created on the client and a matching state is added to the predicted **AbilityState** list. The client activates the ability immediately assuming it got permission from the server
2. The client sends a request to the server to activate the same ability passing along the ability's unique identifier and context
3. The server tries to activate the ability and creates a matching state in the authoritative list
4. The authoritative state is replicated to all clients. If the client that activated the ability receives the new state and it matches the predicted state, the client continues as normal.  If the states don't match, the client triggers a callback where you can implement logic to fix the misprediction
5. For more specific prediction both the client and server can take a **StateSnapshot** of the ability at any time and compare them to fix mispredictions

### What is a **StateSnapshot**?

* A **StateSnapshot** is an arbitrary struct representing the state of the ability when it was taken. e.g. A struct containing references to other actors hit by an ability after an overlap check
    * If an ability is **client predicted** then both the client and server can take a snapshot using a custom struct. The client saves its snapshot to the predicted **Ability State** and the server saves its snapshot to the authoritative **Ability State**
    * When the server replicates the authoritative **Ability State** to the client, the client compares the server's snapshot to its own. If they match, the client continues as normal. If they don't match, the client triggers a callback where you can implement logic to fix the misprediction

Take snapshot
    ![snapshot example 1](../../../images/HLO_Snapshot1.png)  
Decide how to fix misprediction using the identifying snapshot tag  
    ![snapshot example 2](../../../images/HLO_Snapshot2.png)  
Fix misprediction  
    ![snapshot example 3](../../../images/HLO_Snapshot3.png)

{: .note }
State Snapshots are only supported in client predicted abilities.
