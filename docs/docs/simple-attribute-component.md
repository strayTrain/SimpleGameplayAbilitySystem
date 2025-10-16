# SimpleAttributeComponent API

`USimpleAttributeComponent` stores replicated attributes, gameplay tags, and active modifiers for an actor. It also exposes the public surface for applying modifiers, mutating attributes, and broadcasting gameplay events.

## Overview
- Works alongside `USimpleGameplayAbilityComponent`; attach both to the same actor or split across pawn/player-state depending on your replication needs.
- Uses `FFastArraySerializer` containers to replicate attributes, tags, and modifiers efficiently.
- Implements `ISimpleEventReplicator` so modifiers and external systems can publish gameplay events.
- Integrates with a `USimpleTimeSynchronizer` to keep duration timers and prediction snapshots aligned.

### Quick Reference
- **Header:** `Components/SimpleAttributeComponent/SimpleAttributeComponent.h`
- **Base Class:** `UActorComponent`
- **Interfaces:** `ISimpleEventReplicator`
- **Companion Types:** `FFloatAttribute`, `FStructAttribute`, `USimpleAttributeModifier`, `USimpleAttributeSet`

---

## Initialization & Default Data

| Property | Description |
|----------|-------------|
| `AttributeSets` | `USimpleAttributeSet` data assets applied at `BeginPlay` to seed attributes, tags, and modifiers. |
| `DefaultFloatAttributes` / `DefaultStructAttributes` | Inline defaults if you do not want to rely solely on data assets. |
| `DefaultGameplayTags` | Tags granted immediately when the component initializes. |

Combine attribute sets and inline defaults to version your data or patch hotfix values without regenerating assets.

---

## Replicated State Containers

All replicated fields leverage FastArrays:

- `AuthorityGameplayTags` / `LocalGameplayTags`
- `AuthorityFloatAttributes` / `LocalFloatAttributes`
- `AuthorityStructAttributes` / `LocalStructAttributes`
- `AuthorityAttributeModifierStates` / `LocalAttributeModifierStates`
- `AuthorityAttributeModifierMutations` / `LocalAttributeModifierMutations`

Local arrays cache predicted values on clients, while authoritative containers replicate deltas from the server.

---

## Attribute Management API

### Adding & Removing
- `AddFloatAttribute(FFloatAttribute, OverrideIfExists)`
- `RemoveFloatAttribute(AttributeTag)`
- `AddStructAttribute(FStructAttribute, OverrideIfExists)`
- `RemoveStructAttribute(AttributeTag)`

### Querying & Mutating Values
- `HasFloatAttribute(AttributeTag)` / `HasStructAttribute(AttributeTag)`
- `GetFloatAttributeValue(ValueType, AttributeTag, WasFound)` — Choose base or current value.
- `SetFloatAttributeValue(ValueType, AttributeTag, NewValue, Overflow)` — Authority-only setter with overflow reporting.
- `IncrementFloatAttributeValue(ValueType, AttributeTag, Increment, Overflow)` — Handles clamping and overflow accumulation.
- `GetStructAttributeValue(AttributeTag, WasFound)` / `SetStructAttributeValue(AttributeTag, NewValue)`
- `GetFloatAttributeCopy` / `GetStructAttributeCopy` — Snapshot for read-only Blueprint access.

### Handlers & Custom Logic
- `GetAttributeHandler(AttributeTag, HandlerClass)` returns (and instantiates) attribute handler objects for struct attribute mutations.

All setter functions are authority-only; clients rely on replication updates and delegates.

---

## Gameplay Tag API

- `AddGameplayTag(Tag)` / `RemoveGameplayTag(Tag)`
- `HasGameplayTag(Tag)` / `HasAllGameplayTags(Tags)` / `HasAnyGameplayTags(Tags)`
- `GetActiveGameplayTags()` returns a copy of all currently active tags (modifying the copy does not affect the component).

Tag state is reference-counted so multiple systems can add the same tag without stepping on each other.

---

## Modifier Management

- `ApplyAttributeModifier(ModifierClass, Context, InstigatorComponent)` (invoked via Blueprint nodes or actions).
- `CancelAttributeModifier(ModifierID, Context)` / `CancelAttributeModifiersWithTags(TagContainer)`
- `HasAttributeModifier(ModifierClass)` / `HasAttributeModifierWithTag(Tag)` / `HasAttributeModifierInGroup(StackGroupTag)`
- `GetAttributeModifier(ModifierID)` retrieves the live modifier instance (authority only) for inspection.
- `GetPredictedModifierSnapshots()` / `PendingMutationQueue` (internal) keep prediction data for reconciliation.

`USimpleAttributeComponent` automatically manages prediction mutations and resolves mismatches when authoritative results replicate back.

---

## Event Replication

Implements the same API as the ability component:

- `SendEvent`, `SendEventToServer`, `SendEventToClient`, `SendEventToAllClients`
- Reliable RPC path (`ServerSendEvent`, `ClientSendEvent`, `MulticastSendEvent`) with GUID deduplication.
- Events flow through `USimpleEventSubsystem` locally before traveling across the network, keeping clients responsive even while RPCs are in flight.

Use events to notify abilities, UI, or other systems about attribute changes, modifier lifecycle events, or gameplay triggers.

---

## Time Synchronization & Utility Helpers

- `GetServerTime()` delegates to the component’s `USimpleTimeSynchronizer`, returning authoritative time on the server or an estimated clock on clients.
- `GetTimeSynchronizerComponent()` (BlueprintNativeEvent) lets you override which synchronizer instance the component should use—handy if you host a global synchronizer elsewhere.
- `HasAuthority()` is a convenience wrapper for Blueprint checks.

Accurate shared time ensures duration modifiers, cooldowns, and prediction snapshots remain consistent across machines.

---

## Best Practices

- **Co-location:** When possible, keep the attribute and ability components on the same replicated actor to simplify authority checks and event routing. Split them only when you need attributes to outlive a pawn.
- **Prediction:** For client-predicted modifiers, ensure all modifier actions either support prediction or explicitly opt out (`ServerInitiate`, `ServerOnly`) to avoid reconciliation churn.
- **FastArray hygiene:** Allow the component’s internal cleanup routines to manage finished modifiers; avoid manually mutating FastArray containers outside the provided API.

---

## Related Documentation

- [`USimpleGameplayAbilityComponent`](simple-gameplay-ability-component.md)
- [`USimpleAttributeModifier`](simple-attribute-modifier.md)
- [Networking Deep Dive](../guide/networking.md) for replication, prediction, and time sync details.
