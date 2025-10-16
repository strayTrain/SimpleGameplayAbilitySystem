# SimpleGameplayAbilityComponent API

`USimpleGameplayAbilityComponent` orchestrates ability lifecycles, prediction buffers, and event replication. Attach it to the actor that owns abilities (commonly a `PlayerState` or `Pawn`) and feed it ability classes and sets to grant at runtime.

## Overview
- Handles granting/revoking abilities and activation requests.
- Replicates active ability state and prediction snapshots with `FFastArraySerializer`.
- Implements `ISimpleEventReplicator` so abilities and modifiers can publish gameplay events locally or over the network.
- Integrates with a `USimpleTimeSynchronizer` to keep activation timestamps aligned.

### Quick Reference
- **Header:** `Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h`
- **Base Class:** `UActorComponent`
- **Interfaces:** `ISimpleEventReplicator`
- **Companion Classes:** `USimpleGameplayAbility`, `USimpleSubAbility`, `USimpleAbilitySet`

---

## Key Properties

| Property | Type | Description |
|----------|------|-------------|
| `AvatarActor` | `AActor*` (Replicated) | The actor abilities operate on; can differ from the component owner. |
| `AbilitySets` | `TArray<USimpleAbilitySet*>` | Data assets auto-granted at `BeginPlay`. |
| `GrantedAbilities` | `TArray<TSubclassOf<USimpleGameplayAbility>>` (Replicated) | Classes granted manually or via sets. |
| `AuthorityAbilityStates` | `FAbilityStateContainer` (Replicated) | FastArray of authoritative active abilities. |
| `LocalAbilityStates` | `TArray<FAbilityState>` | Client-predicted ability states awaiting reconciliation. |
| `AuthorityAbilitySnapshots` | `FAbilitySnapshotContainer` (Replicated) | Server snapshots used to validate predictions. |
| `LocalPendingAbilitySnapshots` | `TArray<FAbilitySnapshot>` | Queued snapshots that have not yet matched an ability instance. |
| `DeferredSnapshots` | `TArray<FAbilitySnapshot>` | Temp buffer for snapshots that arrived before the ability replicated. |

---

## Granting & Initialization

- `GrantAbility(Class)` / `RevokeAbility(Class)`  
  Server-authoritative functions for changing the component’s ability roster.
- `SetAvatarActor(NewActor)`  
  Update the target actor abilities should manipulate (authority-only).
- `BeginPlay()` automatically grants abilities from `AbilitySets` and binds to ability lifecycle delegates.

> **Tip:** Keep the component on a replicated actor that persists across respawns (e.g., `PlayerState`) so active abilities survive pawn swaps.

---

## Activation API

`USimpleGameplayAbilityComponent` exposes multiple activation paths tailored to prediction and authority needs:

| Function | Use Case |
|----------|----------|
| `ActivateAbility(Class, Context)` | Local activation (no replication). |
| `ActivateAbilityPredicted(Class, Context)` | Client-predicted activation with automatic snapshot reconciliation. |
| `ActivateAbilityServerInitiated(Class, Context)` | Client requests; server validates and broadcasts authoritative start. |
| `CancelAbility(AbilityInstanceID, Context)` | Server-side cancellation. |
| `CancelAbilityPredicted(AbilityInstanceID, Context)` | Client predicts cancellation and reconciles with server. |
| `CancelAbilitiesWithTags*` / `CancelAbilitiesWithClass*` | Bulk cancellation using tag or class filters (predicted + authoritative variants). |

All activation/cancellation calls accept `FInstancedStruct` payloads so you can pass typed context data between caller and ability.

---

## Event Replication

The component implements `ISimpleEventReplicator` and provides built-in RPCs:

- `SendEvent`, `SendEventToServer`, `SendEventToClient`, `SendEventToAllClients`
- Backed by internal helpers (`ServerSendEvent`, `ClientSendEvent`, `MulticastSendEvent`) that attach a GUID to each event to avoid duplicate delivery when multicasting.
- Local events are forwarded immediately to `USimpleEventSubsystem`, while remote events travel through reliable RPCs.

Use events to decouple abilities, modifiers, and UI systems without adding new RPCs.

---

## Prediction & Snapshot Utilities

- `AddGameplayAbilitySnapshot(AbilityID, SnapshotData)` records a state sample for later reconciliation.
- `ResolveLocalAbilityState`, `ProcessDeferredSnapshots`, and `TryResolveSnapshot` keep predicted arrays synchronized when server updates arrive.
- `CleanupOldAbilityStates()` prunes inactive states roughly 30 seconds after completion to keep replication arrays lean.
- `GetServerTime()` proxies the shared time synchronizer for both authoritative and client-estimated timestamps.

Internally, FastArray callbacks (`ClientOnAbilityStateAdded/Changed/Removed`) fire whenever authoritative data changes, triggering UI delegates or prediction resolution.

---

## Utility Helpers

- `HasAuthority()` / `IsOwnedByLocalPlayer()` — Convenient authority and possession checks for Blueprints.
- `IsAnyAbilityActive()` — Quickly determine if the component currently has running abilities.
- `GetAbilityInstanceByID` / `GetAbilityInstanceByClass` / `GetAbilityStateByID` — Direct access to live ability instances or their replicated state.
- `GetTimeSynchronizerComponent()` — Blueprint-extensible hook to point the component at a custom `USimpleTimeSynchronizer` instance.

---

## Interaction with Other Systems

- **Attributes:** Abilities frequently call into `USimpleAttributeComponent` for statistics, tags, and applying modifiers. Ensure both components share the same time synchronizer for accurate cooldowns and durations.
- **Modifiers:** Activation payloads, cancellation contexts, and event payloads travel through `FInstancedStruct`, keeping the API data-driven and strongly typed.
- **Events:** Abilities and modifiers can use `SendEvent*` to broadcast tag-identified events with optional payloads. Listeners filter by tag/domain to avoid noisy updates.

---

## Related Documentation

- [`USimpleGameplayAbility`](simple-gameplay-ability.md) — Ability class API.
- [`USimpleAttributeComponent`](simple-attribute-component.md) — Attribute and modifier host.
- [Networking Deep Dive](../guide/networking.md) — Prediction, FastArray replication, and time synchronization details.
