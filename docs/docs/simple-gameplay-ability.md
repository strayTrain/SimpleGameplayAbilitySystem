# SimpleGameplayAbility API

`USimpleGameplayAbility` is the blueprintable building block for authored gameplay actions in SimpleGAS. Each instance tracks its own lifecycle, handles prediction reconciliation, and provides helper utilities for working with attributes, tags, and sub-abilities.

## Overview
- Lives inside a `USimpleGameplayAbilityComponent`.
- Instanced per activation (`SingleInstance` or `MultipleInstances`).
- Fully blueprintable; override key hooks such as `CanActivate`, `OnActivate`, and `OnEndAbility`.
- Integrates with prediction snapshots and shared time synchronization.

### Quick Reference
- **Header:** `SimpleAbility/SimpleGameplayAbility/SimpleGameplayAbility.h`
- **Base Class:** `USimpleAbilityBase`
- **Primary Component:** `USimpleGameplayAbilityComponent`
- **Key data types:** `FInstancedStruct`, `FGuid`, `FGameplayTagContainer`

---

## Lifecycle & Network Roles

Abilities activate through a consistent flow:

1. **Grant (optional):** `OnGranted` fires when the owning component grants the class.
2. **Request:** `ActivateAbility*` is called on the component with context data.
3. **Validation:** `CanActivate` runs alongside tag checks and context validation.
4. **Activation:** `OnActivate` executes gameplay logic; `TakeStateSnapshot` enables prediction.
5. **Resolution:** Server snapshots arrive via `OnServerSnapshotReceived` for reconciliation.
6. **Cleanup:** `EndAbility` or `CancelAbility` finalizes state and clears temporary tags.

`EAbilityNetworkRole` returned by `GetNetworkRole()` describes whether the ability was started locally, predicted, or authoritatively on the server.

---

## Configuration Properties

| Property | Type | Description |
|----------|------|-------------|
| `InstancingPolicy` | `EAbilityInstancingPolicy` | Choose `SingleInstance` or `MultipleInstances` to control coexistence. |
| `RequireGrantToActivate` | `bool` | If `true`, the ability must be granted before activation requests succeed. |
| `ActivationRequiredTags` / `ActivationBlockingTags` | `FGameplayTagContainer` | Tag filters evaluated on the owning `USimpleAttributeComponent`. |
| `RequiredContextType` | `UScriptStruct*` | Optional context payload type check. |
| `AvatarTypeFilter` | `TArray<TSubclassOf<AActor>>` | Optional whitelist for the component’s avatar actor. |
| `AbilityTags` | `FGameplayTagContainer` | Categorize the ability; used by cancel/filter helpers. |
| `TemporarilyAppliedTags` | `FGameplayTagContainer` | Tags applied while the ability is active. |
| `PermanentlyAppliedTags` | `FGameplayTagContainer` | Tags applied on activation and left behind when the ability ends. |

All configuration properties are editable in Blueprint or defaults.

---

## Sub-Ability Support

Sub-abilities encapsulate reusable fragments such as montage playback. Use the provided helpers to work with them safely:

- `ActivateSubAbility(Class, Context, ActivationResult)` spawns and runs a `USimpleSubAbility`, returning the instance for coordination.
- `GetSubAbilityInstance(Class)` retrieves an existing sub-ability created by this ability.
- `OnSubAbilityEnded` / `OnSubAbilityCancelled` automatically clean up tracked sub-abilities when the parent ends.

Sub-abilities inherit the parent’s network role; they do not replicate independently.

---

## Attribute & Component Access

```cpp
USimpleGameplayAbilityComponent* GetAbilityComponent() const;
USimpleAttributeComponent*      GetAttributeComponent();
AActor*                         GetAvatarActor() const;
```

- Override `GetAttributeComponent()` if your ability and attribute components live on different actors (e.g., `Pawn` vs `PlayerState`).
- `GetAvatarActorAs<T>` casts the avatar and returns an `IsValid` flag for Blueprint safety.

---

## Prediction & Time Utilities

- `TakeStateSnapshot(SnapshotData, OnResolved)` queues a snapshot for prediction reconciliation. Call it whenever you start client-predicted work that must agree with the server (montages, projectiles, etc.).
- `OnServerSnapshotReceived(SnapshotCounter, AuthorityData, LocalData)` is invoked automatically on clients to compare results and apply corrections.
- `GetActivationTime()` returns the shared server time (from the component’s `USimpleTimeSynchronizer`) when the ability started.
- `GetActivationDelay()` measures latency between local activation and the server’s authoritative start—ideal for fast-forwarding montages or timers.

---

## Activation & State Helpers

- `CanActivate` (BlueprintNativeEvent) — Override to inject custom validation prior to activation.
- `OnGranted` (static and instance variants) — Hook for granting-time setup such as binding inputs.
- `HasAuthority()` and `GetNetworkRole()` — Inspect execution authority for branching logic.
- `AbilityEndedInternal` / `PreActivateInternal` — Base overrides that manage tag application, timer initialization, and sub-ability cleanup. Avoid overriding unless extending in C++; use Blueprint events instead.

---

## Common Blueprint Pattern

```mermaid
flowchart TD
    Input[Input / Condition] --> Call[ActivateAbilityPredicted]
    Call --> Ability[SimpleGameplayAbility.OnActivate]
    Ability --> Snapshot[TakeStateSnapshot]
    Ability --> SubAbility[ActivateSubAbility (Play Montage)]
    Ability --> Tags[Apply TemporarilyAppliedTags]
    Snapshot -->|Server Snapshot Arrives| Resolve[OnServerSnapshotReceived]
    Resolve --> Correction[Adjust montage/timers if mismatch]
    Ability --> End[EndAbility / CancelAbility]
    End --> Cleanup[Tags removed, sub-abilities cleaned]
```

---

## Related Types

- `USimpleGameplayAbilityComponent` — Grants, activates, and replicates ability instances.
- `USimpleSubAbility` — Minimal, non-replicating helper abilities.
- `USimpleAttributeComponent` — Provides attributes, tags, and modifier hooks for ability logic.

For replication and networking details, see the [Networking Deep Dive](../guide/networking.md).
