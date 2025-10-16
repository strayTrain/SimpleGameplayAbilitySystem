# Why use SimpleGAS?

## The Problem with Multiplayer Games

Building a multiplayer game in Unreal Engine is like assembling a distributed machine — every moving part must stay synchronized across multiple computers.

You want **abilities** (attacks, dashes, shields), their **effects** (damage, stamina drain, healing), and consistent **network replication**.
At first, it seems simple:

* You make a Blueprint dash that plays a montage and spawns particles.
* You add RPCs so it works in multiplayer.
* You access the `Stamina` variable for costs and check `IsStunned` to block dashes.
* You move validation to the server for authority — now the dash feels laggy.
* You compensate by running it locally first, sending an RPC, and rolling back if the server disagrees.

Soon you’re managing:

* **Manual replication** for every ability and state
* **Scattered logic** across multiple components and checks
* **Tight coupling** between abilities, attributes, and conditions
* **Copy-pasted code** for animations and effects
* **Desync issues** when the client and server disagree

One missed RPC or forgotten condition and the system breaks. What started as a simple dash turns into a fragile, hard-to-extend web of replicated logic.

---

## Epic’s Solution

Epic’s **Gameplay Ability System (GAS)** solves these challenges for large-scale projects like *Fortnite* and *Paragon*. It provides a complete framework for attributes, abilities, tags, prediction, and replication.

But that power comes with trade-offs:

* **Steep learning curve** — dozens of interconnected classes and concepts (`UAbilitySystemComponent`, `UGameplayEffect`, `FGameplayAbilitySpec`, etc.)
* **High boilerplate** — even a simple ability requires multiple assets and C++ hooks
* **C++ dependency** — common extensions push you into native code
* **All-or-nothing architecture** — difficult to adopt partially or retrofit into an existing project

GAS is ideal for large teams building complex, interconnected systems — but overkill for smaller games or prototypes that just need “abilities and stats that replicate.”

---

## The SimpleGAS Approach

**SimpleGAS** rethinks the problem by asking:
*What’s the smallest, cleanest set of tools you need to build reusable, replicated gameplay logic without boilerplate?*

It focuses on **three core systems:**

1. **Abilities** – Self-contained, replicated, and predictable gameplay actions with clear lifecycles.
2. **Attributes** – Replicated stats (float or struct) with built-in tag management and event dispatching.
3. **Events** – A lightweight pub/sub system for communication between components without tight coupling.

No `GameplayEffects`. No `ExecutionCalculations`. No nested attribute specs.
Just abilities, modifiers, and tags — clean, predictable, and multiplayer-ready.

---

### Example: Fireball Ability

Want a fireball that costs mana and applies a burn?

1. **Create a Blueprint ability** with a mana requirement and a blocking tag check.
2. **Apply a modifier** to reduce mana (one node).
3. **Spawn the projectile** and apply a burn modifier on hit.
4. **The burn ticks** and applies damage every second — fully replicated.

No C++, no extra assets, no manual RPCs.
It’s all handled by the framework.

---

## Choosing the Right Tool

**Use GAS if:**

* You’re building a large RPG or MOBA with deep stat systems
* You have a team that can dedicate time to mastering it
* You need advanced gameplay cueing, stacking, and execution logic

**Use SimpleGAS if:**

* You’re a solo dev or small team shipping quickly
* Designers need to build abilities directly in Blueprints
* You want a clean, minimal system that “just works” in multiplayer

SimpleGAS isn’t a replacement for GAS — it’s a streamlined alternative.
You trade complexity for clarity, and gain a framework you can learn in an afternoon and extend in a week.