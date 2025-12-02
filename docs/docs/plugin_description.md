# SimpleGameplayAbilitySystem — LLM Reference

Use this file as drop-in context for model discussions. It summarizes how the plugin is built, what the major classes do, and where to look in the source code. The implementation leans on gameplay tags, instanced structs, and FastArray replication to keep abilities, attributes, and events in sync across the network.

---

## Mental Model
- Put **two components** on the owning actor: `USimpleGameplayAbilityComponent` orchestrates abilities/prediction, and `USimpleAttributeComponent` owns replicated attributes, tags, and modifiers.
- Abilities are UObject-based (`USimpleGameplayAbility`) with a small lifecycle; they can spawn **sub-abilities** for reusable fragments.
- Attribute modifiers are UObject bundles that run **modifier actions** (change attributes, trigger abilities, send events, etc.) and can tick/stack.
- Both components implement `ISimpleEventReplicator`, forwarding gameplay-tagged events through `USimpleEventSubsystem` locally and via RPC when asked.
- Timekeeping uses `USimpleTimeSynchronizer` (extended version by default) so prediction snapshots and modifier ticks align to server time.

---

## Modules & Important Files
- Runtime module: `Source/SimpleGameplayAbilitySystem/`
  - Component code: `Components/SimpleGameplayAbilityComponent/`, `Components/SimpleAttributeComponent/`
  - Abilities: `SimpleAbility/`
  - Event bus: `SimpleEventSubsystem/`
  - Default tags: `DefaultTags/`
- Debugger helpers: `Source/SimpleGameplayAbilitySystemDebugger/` (Gameplay Debugger categories for ability/attribute/modifier state)
- Data assets: `DataAssets/AbilitySet`, `DataAssets/AttributeSet`
- Editor helpers: `Source/SimpleGameplayAbilitySystemEditor/` (BP libraries, node helpers)

---

## Core Runtime Pieces

### SimpleGameplayAbilityComponent (`.../Components/SimpleGameplayAbilityComponent/`)
- Responsibilities: grants abilities (via `AbilitySets` or `GrantAbility`), activates them, tracks replicated ability state/snapshots, and routes events through `ISimpleEventReplicator`.
- Key properties: `AvatarActor` (decouples owner from the actor abilities target), `GrantedAbilities` (replicated `TArray`), `AuthorityAbilityStates` and `AuthorityAbilitySnapshots` (FastArray replication), local mirrors for prediction.
- Activation entry points:
  - `ActivateAbility` / `ActivateAbilityWithContext`: local only (no replication).
  - `ActivateAbilityPredicted*`: client-predicted; captures local state, fires immediately, then RPCs `ServerActivateAbility` for confirmation.
  - `ActivateAbilityServerInitiated*`: asks the server to run, then replicates to clients.
  - Instancing policy: `SingleInstance` abilities cancel a running instance on re-activation; `MultipleInstances` spawn new objects and are cleaned up on end/cancel callbacks.
- Ability state replication: FastArray items contain `AbilityID`, class, status (`PreActivation`, `ActivationSuccess`, `ActivationFailed`, `Ended`, `Cancelled`), activation/end timestamps, and context. The component resolves server updates against local predicted state and cancels/ends local abilities when the server disagrees.
- Snapshots for prediction: `AddGameplayAbilitySnapshot` records `AbilityID`, mutable `SnapshotData`, and timestamps. Clients keep `LocalPendingAbilitySnapshots` and a `DeferredSnapshots` queue for snapshots that arrive before the ability instance exists; abilities resolve mismatch via `USimpleGameplayAbility::OnServerSnapshotReceived`.
- Event replication: `SendEvent` (local), `SendEventToServer`, `SendEventToClient`, `SendEventToAllClients`. Events carry `EventTag`, optional `DomainTag`, payload `FInstancedStruct`, sender, and listener filter. Event IDs de-dup duplicate deliveries; IDs expire after ~30s.
- Utilities: `GetSimpleAttributeComponent_Implementation` searches owner, then PlayerState; `GetServerTime()` uses the time synchronizer component if present; `IsOwnedByLocalPlayer` supports Pawn or PlayerState ownership.

### SimpleGameplayAbility (`.../SimpleAbility/SimpleGameplayAbility/`)
- Derived from `USimpleAbilityBase` (minimal lifecycle: `ActivateAbility` → `OnActivate`/tick → `EndAbility`/`CancelAbility`). `Initialize` caches the ability/attribute components and stamps `AbilityID` and activation time.
- Activation requirements: optional grant check (`RequireGrantToActivate`), required/blocking gameplay tags on the attribute component (`ActivationRequiredTags`, `ActivationBlockingTags`), optional `RequiredContextType`, and `AvatarTypeFilter` to restrict avatar actor types. Fails `CanActivate` with logged reasons.
- Tags: `TemporarilyAppliedTags` are added on activate and removed on end/cancel; `PermanentlyAppliedTags` must be removed manually by gameplay code when desired.
- Sub-abilities: `ActivateSubAbility` creates `USimpleSubAbility` instances, hooks lifecycle delegates, and enforces their cancellation policy (cancel/end when the parent stops). Sub-abilities never replicate on their own.
- Prediction helpers: `TakeStateSnapshot` pushes snapshot data through the ability component; `OnServerSnapshotReceived` compares authority vs. local snapshot and executes a resolution delegate only when data differs. `GetActivationTime`/`GetActivationDelay` expose server-time aligned timing for animation offsetting.

### SimpleSubAbility (`.../SimpleAbility/SimpleSubAbility/`)
- Lightweight, non-replicated ability fragments with their own `RequiredContextType` and `AvatarTypeFilter`. `CancellationPolicy` controls how they react to parent end/cancel.
- Can emit events tied to the parent ability ID via `SendEvent*` helpers, letting parent/other systems listen through the event bus.

### SimpleAttributeComponent (`.../Components/SimpleAttributeComponent/`)
- Owns replicated gameplay tags (`FGameplayTagCounterContainer`), float attributes, struct attributes, and the live modifier stack. Default values come from component arrays and `USimpleAttributeSet` data assets on `BeginPlay` (server only).
- Gameplay tags: reference-counted add/remove (`AddGameplayTag`, `RemoveGameplayTag`) so multiple systems can hold the same tag safely. Both authority and local arrays exist; FastArray callbacks keep clients in sync.
- Float attributes: store `BaseValue`, `CurrentValue`, and `FValueLimits` (min/max optional clamps for base/current). Setters clamp to limits, emit overflow values, and broadcast `OnFloatAttributeBaseValueChanged` / `OnFloatAttributeCurrentValueChanged` delegates.
- Struct attributes: hold `UScriptStruct*` plus `FInstancedStruct` payloads. Optional `USimpleAttributeHandler` per attribute can validate type, compute modification tags, and provide helper get/set. Handlers are instanced and cached.
- Events: multicast delegates for add/change/remove of each attribute type and for gameplay tag add/remove. Component also implements `ISimpleEventReplicator` with the same event routing/dedup scheme as the ability component.
- Time sync + authority checks mirror the ability component (`GetServerTime`, `HasAuthority`).

### Attribute Modifiers (`.../Components/SimpleAttributeComponent/SimpleAttributeModifier/`)
- Concept: UObject that wraps a **stack of modifier actions**. Life cycle is configurable by `DurationType` (`Instant`, `SetDuration`, `InfiniteDuration`) plus optional tick interval. A modifier is applied via the attribute component to a target component.
- Requirements & tags: `TargetRequiredTags`, `TargetBlockingTags`, `TargetBlockingModifierTags`, optional `RequiredContextType`, and avatar filters enforced before apply. `ModifierTags` classify the modifier; `TemporarilyAppliedTags`/`PermanentlyAppliedTags` are added to the target while active.
- Stacking: optional `StackGroupTag`, `OnReapplication` (allow multiple, reset/extend/refresh), `MaxStacksInGroup` with overflow strategies (`DenyNew`, replace oldest/newest, extend oldest).
- Scratch pad: `FAttributeModifierActionScratchPad` holds tags, numeric values, and struct blobs shared across actions in the stack. Defaults can be pre-filled via `InitialScratchPadValues`; ticks can reset the pad or accumulate state.
- Prediction: client-predicted applications capture an attribute snapshot (`CaptureAttributeSnapshot`) before applying. Server replication of `FAttributeModifierState` (`Applied`, `Cancelled`, `Ended`) reconciles by confirming predicted modifiers, rolling back with snapshots when rejected, or ending locally when the server ends. Server also replicates `FAttributeModifierMutation` (action stack results) so predicted actions can run `OnClientReceivedServerActionsResult` for correction.
- Actions: each `UModifierAction` chooses a prediction policy (`PredictIfPossible`, etc.), reacts to modifier lifecycle events (default triggers include `AttributeModifier.Applied/Ticked/Ended/Cancelled`), and can subscribe to SimpleEventSubsystem via function references. Built-in actions live under `ModifierActions/`:
  - `ChangeFloatAttributeAction` & `ChangeStructAttributeAction`: mutate attributes with operations (add/subtract/multiply/divide/power/override/custom), overflow capture, and support for sourcing values from magnitude, scratch pad, instigator/target attributes, or custom functions.
  - `ApplyAttributeModifierAction` / `CancelModifierAction`: apply or cancel other modifiers (instigator or target).
  - `ActivateGameplayAbilityAction` / `CancelAbilityAction`: trigger/cancel abilities via the ability component with configurable prediction policy.
  - `SendSimpleEventAction`: emit gameplay-tagged events through the event bus.
  - `RuntimeAction`: Blueprint/C++ hook to execute arbitrary logic while still participating in prediction result reconciliation.
- Lifecycle delegates: `OnModifierApplied`, `OnActionStackApplied` (mirrored as mutations for replication), `OnAttributeModifierEnded`, `OnAttributeModifierCancelled`. Cleanup (`OnCleanupModifier`) runs after end/cancel and when server retention expires.

### Event Layer (`.../SimpleEventSubsystem/` + `.../Interfaces/SimpleEventReplicator.h`)
- `USimpleEventSubsystem` (GameInstance subsystem) delivers local events to listeners filtered by event/domain tags, optional payload type, sender inclusion list, and exact/partial tag matching. Returns a subscription GUID and supports cancellation by GUID, filters, or all events for a listener.
- `ISimpleEventReplicator` is implemented by both core components. They create unique event IDs, optionally send via RPC (server/client/multicast), and deduplicate to avoid loopback storms. Domain tags help scope listeners (`FDefaultTags::DomainAbility`, `DomainAttributeModifier`).
- Default gameplay tags (`Source/SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h`) include modifier lifecycle events, sub-ability end/cancel tags, and scratch pad markers for overflow/runtime action results.

### Time Synchronization (`.../Components/SimpleTimeSynchronizerComponent/ExtendedTimeSynchronizer/`)
- `USimpleTimeSynchronizerExtended` samples round-trip times, rejects outliers, and smooths clock offset. Components default to asking this for `GetServerTime()` and fall back to `GameState->GetServerWorldTimeSeconds()`.
- Configurable sample buffer, sync intervals, burst sampling on join, RTT-based outlier rejection, and exposed metrics (`CurrentClockOffset`, `CurrentRTT`).

---

## Networking & Prediction Rules
- FastArrays everywhere: ability states/snapshots and attribute states/mutations replicate via `FFastArraySerializer`, firing bound delegates on clients to resolve prediction and spawn instances if needed.
- Ability prediction: clients can instantly activate, store local state, and later reconcile against server state changes. Snapshots provide finer-grained reconciliation for arbitrary ability data.
- Modifier prediction: clients snapshot attributes before applying; if the server cancels, the component restores the snapshot. Server-sent mutation results let clients correct predicted action outputs/scratch pads.
- State retention/cleanup: server trims ended ability states after ~5s and snapshots after ~10s; attribute modifier states/mutations and predicted snapshots clean up after up to ~30s (see `MaxStateRetentionTime` in the attribute component).
- Event replication uses GUID deduplication and a 30s expiry to prevent replay storms.

---

## Blueprint Surface
- Both components are Blueprint-spawnable (`BlueprintSpawnableComponent`), with most APIs exposed (`ActivateAbility*`, `ApplyAttributeModifier*`, cancel helpers, etc.).
- Context-friendly custom thunks allow any struct input for ability/modifier contexts; C++ variants accept `FInstancedStruct` directly.
- Async helpers under `AsyncActions/`:
  - `WaitForAttributeChange`: listens for float/struct attribute updates.
  - `WaitForGameplayTag`: waits on tag add/remove.
  - `WaitForSubAbility` / `WaitForClientSubAbility`: await sub-ability results for server/client respectively.
  - `WaitForSimpleEvent`: listens for `USimpleEventSubsystem` events.
- Function selector helpers (e.g., action Simple Event filters) live in `BlueprintFunctionLibraries/FunctionSelectors/`; node utilities in `BlueprintFunctionLibraries/NodeHelpers/`.

---

## Data Assets & Defaults
- `USimpleAbilitySet` (`DataAssets/AbilitySet/SimpleAbilitySet.h`): bundle of ability classes to auto-grant on `BeginPlay` (server).
- `USimpleAttributeSet` (`DataAssets/AttributeSet/SimpleAttributeSet.h`): sets of float/struct attributes to seed the attribute component.
- Default gameplay tags are loaded from `Config/Tags/` via module startup; see `DefaultTags.h` for canonical accessors.

---

## Debugging & Tooling
- Gameplay Debugger categories (`Source/SimpleGameplayAbilitySystemDebugger/Public/`):
  - Ability component view: shows ability states, network role, timestamps, pending/deferred snapshots.
  - Attribute component view: lists attributes/tags and recent changes.
  - Attribute modifier view: displays active modifiers with IDs, class names, and status.
- Logs: `LogSimpleGAS` (declared in `Module/SimpleGameplayAbilitySystem.h`) with helpers such as `SIMPLE_LOG(WorldContext, Msg)` prefixing `[SERVER]/[CLIENT]`.

---

## Common Usage Pattern
1. Add `USimpleAttributeComponent` and `USimpleGameplayAbilityComponent` to the owning actor (often `PlayerState` so it survives possession). Optionally attach `USimpleTimeSynchronizerExtended` for better prediction.
2. Seed attributes/tags via component defaults or `USimpleAttributeSet` assets; seed abilities via `AbilitySets` or `GrantAbility` on begin play (server).
3. Activate abilities from input using `ActivateAbilityPredicted` (client) or `ActivateAbilityServerInitiated` (server), passing a context struct when needed.
4. Inside abilities, call sub-abilities for reusable blocks and take snapshots if you need prediction reconciliation.
5. Apply attribute modifiers from abilities or other systems; pick duration/stacking rules and modifier actions. Use prediction variants for responsive client-side feedback.
6. Use the event subsystem for decoupled messaging (UI, VFX hooks, or inter-system signals) with domain tags and payload filters.

---

## Gotchas & Tips
- `RequireGrantToActivate` is enforced per ability; ensure abilities are granted on the server or disable the flag for one-off spawns.
- Single-instance abilities auto-cancel a running copy when reactivated. Multiple-instance abilities are manually cleaned up on end/cancel callbacks.
- Set `AvatarActor` on the ability component when the component lives off the pawn (e.g., on `PlayerState`) so abilities can fetch the correct actor.
- Struct attributes need a valid `StructType`; handlers must match this type or the component will log and refuse to modify.
- Prediction relies on `GetServerTime()`; add the time synchronizer component in networked games for stable activation delays and modifier ticks.
- Event payloads use `FInstancedStruct`; keep payload types small and deterministic when sending across the network.

---

## File Map (fast lookup)
- Ability component: `Source/SimpleGameplayAbilitySystem/Components/SimpleGameplayAbilityComponent/SimpleGameplayAbilityComponent.h/.cpp`
- Ability classes: `Source/SimpleGameplayAbilitySystem/SimpleAbility/SimpleGameplayAbility/`, base in `SimpleAbilityBase/`, sub-abilities in `SimpleSubAbility/`
- Attribute component: `Source/SimpleGameplayAbilitySystem/Components/SimpleAttributeComponent/`
- Attribute modifiers & actions: `.../SimpleAttributeComponent/SimpleAttributeModifier/`
- Event system: `Source/SimpleGameplayAbilitySystem/SimpleEventSubsystem/`, interface in `Interfaces/SimpleEventReplicator.h`
- Time sync: `Source/SimpleGameplayAbilitySystem/Components/SimpleTimeSynchronizerComponent/`
- Default tags: `Source/SimpleGameplayAbilitySystem/DefaultTags/DefaultTags.h`
- Gameplay debugger: `Source/SimpleGameplayAbilitySystemDebugger/`

