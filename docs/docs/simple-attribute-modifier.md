# SimpleAttributeModifier API

`USimpleAttributeModifier` encapsulates buffs, debuffs, damage-over-time effects, and any other attribute-driven gameplay changes. Modifiers bundle configuration, lifetime tracking, modifier actions, and event hookups so you can author complex effects without hand-writing replication code.

## Overview
- Hosted by `USimpleAttributeComponent` and replicated with `FFastArraySerializer`.
- Supports instant, fixed-duration, or infinite lifetimes with optional tick intervals.
- Applies one or more `UModifierAction` objects that perform the actual work (damage, healing, ability triggers, etc.).
- Integrates with prediction so clients can apply modifiers locally and reconcile against server results.

### Quick Reference
- **Header:** `Components/SimpleAttributeComponent/SimpleAttributeModifier/SimpleAttributeModifier.h`
- **Base Class:** `UObject`
- **Companion Types:** `UModifierAction`, `FAttributeModifierState`, `FAttributeModifierMutation`
- **Key Enums:** `EAttributeModifierDurationType`, `EDurationTickTagRequirementBehaviour`, `EStackGroupOverflowBehavior`

---

## Configuration Properties

| Property | Type | Purpose |
|----------|------|---------|
| `DurationType` | `EAttributeModifierDurationType` | Choose `Instant`, `SetDuration`, or `InfiniteDuration`. |
| `Duration` | `float` | Lifetime in seconds for `SetDuration` modifiers. |
| `TickInterval` | `float` | Frequency (seconds) for re-running the action stack when non-instant. |
| `TickRequirementsFailedBehaviour` | `EDurationTickTagRequirementBehaviour` | Decide whether to skip or cancel when tag requirements fail mid-tick. |
| `ResetScratchPadOnTick` | `bool` | Clear the shared scratchpad between ticks or keep persistent values. |
| `ModifierTags` | `FGameplayTagContainer` | Classify the modifier for filtering and UI. |
| `TemporarilyAppliedTags` / `PermanentlyAppliedTags` | `FGameplayTagContainer` | Tags to add while active (temporary) or after ending (permanent). |
| `TargetRequiredTags` / `TargetBlockingTags` / `TargetBlockingModifierTags` | `FGameplayTagContainer` | Tag-based gating for application. |
| `bUseStackGroup`, `StackGroupTag`, `MaxStacksInGroup`, `OverflowBehavior`, `OnReapplication` | Stack management controls for duration/infinite modifiers. |
| `ModifierActions` | `TArray<UModifierAction*>` | Inline instanced objects defining the work performed. |

All configuration fields are Blueprint-editable with contextual visibility based on duration settings.

---

## Lifecycle Callbacks

- `CanApplyModifier()` — Validation hook before the modifier runs. Respect tag checks, cooldowns, or resource availability here.
- `OnPreApplyModifierActions()` / `OnPostApplyModifierActions()` — Override to add setup/teardown around the action stack run.
- `OnModifierEnded(EndingStatus, EndingContext)` — Called when the modifier completes normally.
- `OnModifierCancelled(EndingStatus, EndingContext)` — Called when the modifier ends early (e.g., cleansed).
- `OnCleanupModifier()` — Final cleanup after end/cancel; default implementation unsubscribes SimpleEvent listeners.
- `OnModifierApplied`, `OnActionStackApplied`, `OnAttributeModifierEnded`, `OnAttributeModifierCancelled` — BlueprintAssignable delegates for external observers.

Lifecycle helpers automatically respect prediction. If the server rejects a predicted modifier, `OnClientReceivedServerActionsResult` delivers both server and client results so you can resolve differences.

---

## Runtime API

| Function | Description |
|----------|-------------|
| `InitializeModifier(NewID, InstigatorComponent, TargetComponent, Magnitude, Context, DoesReplicate)` | Internal setup called by the attribute component. |
| `ApplyModifier()` | Runs validation, applies tags, and executes the modifier action stack. |
| `EndModifier(StatusTag, EndingContext)` | Ends gracefully, firing `OnModifierEnded`. |
| `CancelModifier(StatusTag, EndingContext)` | Ends early, firing `OnModifierCancelled`. |
| `GetRemainingDuration()` / `ExtendDuration()` / `SetRemainingDuration()` | Manage duration modifiers at runtime. |
| `GetActivationTime()` | Returns the shared server time when this modifier started (via the owning component’s time synchronizer). |
| `TriggerActionsForEvents(EventTags)` | Manually invoke actions listening for specific tags. |
| `GetModifierActionScratchPad()` | Access shared scratch data for coordination between actions. |

Modifiers store their original context as `ModifierContext`, making it easy to inspect payload data when events fire or ticks run.

---

## Prediction & FastArray Integration

`USimpleAttributeComponent` keeps authoritative modifier state in `FAttributeModifierStateContainer` and predicted mutations in `FAttributeModifierMutationContainer`. When a client applies a modifier predicted:

1. The modifier runs locally and records a snapshot.
2. A `PendingMutation` entry waits for the server’s response.
3. The server replays the modifier, replicates the authoritative state, and sends action results.
4. `OnClientReceivedServerActionsResult` compares server/client scratch pads and results, letting actions reconcile via `OnClientPredictedCorrection`.

Because underlying FastArrays only replicate changed entries, high-frequency modifiers (bleeds, HoTs) stay bandwidth-friendly.

---

## Working with Modifier Actions

`UModifierAction` subclasses perform the actual gameplay work. The base class provides:

- `PredictionPolicy` (`PredictIfPossible`, `ServerInitiate`, etc.).
- `EventTriggers` — Gameplay tags that trigger the action when broadcast by the modifier.
- `SimpleEventTriggers` — Optional function reference for additional filtering.
- Utility methods for handling the shared scratchpad (tags and float values).

### Built-In Actions

| Action | Purpose |
|--------|---------|
| `UChangeFloatAttributeAction` | Modify float attributes using additive, multiplicative, or custom functions. |
| `UChangeStructAttributeAction` | Mutate struct attributes through registered handlers. |
| `UActivateGameplayAbilityAction` | Trigger abilities on the instigator or target component. |
| `UApplyAttributeModifierAction` | Chain or nest additional modifiers. |
| `UCancelAbilityAction` | Cancel active abilities by class or tags. |
| `UCancelModifierAction` | Remove modifiers matching class or tags (e.g., cleanse). |

Extend `UModifierAction` in Blueprint or C++ to add bespoke behavior—spawn projectiles, push payloads into the SimpleEvent system, or drive custom VFX.

---

## Event Integration

Modifiers automatically register with `ISimpleEventReplicator` when they need to respond to gameplay events. Use `EventTriggers` or `SimpleEventTriggers` to react to:

- `AttributeModifierApplied` (default)
- `AttributeModifierTicked`
- Custom gameplay tags broadcast by abilities, other modifiers, or subsystems

This model keeps modifiers loosely coupled and replicates event payloads only when needed.

---

## Related Documentation

- [`USimpleAttributeComponent`](simple-attribute-component.md) — Host component that stores attributes, modifiers, and tags.
- [`USimpleGameplayAbility`](simple-gameplay-ability.md) — Abilities that frequently apply or remove modifiers.
- [Networking Deep Dive](../guide/networking.md) — Details on modifier prediction, FastArray replication, and event flow.
