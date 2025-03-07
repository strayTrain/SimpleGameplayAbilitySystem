---
title: Attribute Modifiers
layout: home
parent: Concepts
nav_order: 7
---

# Attribute Modifiers

Attribute Modifiers are the workhorses of SimpleGAS - they're how you actually change attributes during gameplay. Need to deal damage, apply a buff, or add a status effect? Attribute Modifiers are your go-to tool.
Think of Attribute Modifiers as specialized tools that can read, change, and track changes to attributes. They come in two flavors:

- **Instant Modifiers**: Apply their changes immediately and are done (like dealing damage)
- **Duration Modifiers**: Stick around for a while, doing their thing repeatedly (like poison damage over time)

At their core, Attribute Modifiers take an attribute and say "change this value in this way." They can:

- Add or subtract from an attribute
- Multiply an attribute by some value
- Override an attribute with a new value

But they can do much more than just basic math! Modifiers can:

- Chain together multiple attribute changes
- Trigger side effects like visual effects or sounds
- Activate other abilities
- Apply gameplay tags to mark states
- Remove or cancel other modifiers

![Attribute modifier in the editor](../../images/BS_AM_Variables.png)

<details markdown="1">
  <summary>Config</summary>

When you create an Attribute Modifier, you'll configure these key settings:

- **Modifier Type**: Determines the basic behavior
  - **Instant**: Apply once and you're done (like a one-time healing spell)
  - **Duration**: Stick around for a set time, possibly applying effects repeatedly (like a buff)

- **Modifier Application Policy**: Controls how the modifier works in multiplayer
  - **Apply Server Only**: Only the server applies the actual attribute changes
  - **Apply Server Only But Replicate Side Effects**: Server changes attributes, but clients can still show effects
  - **Apply Client Predicted**: Client applies changes immediately assuming server will agree, then corrects if needed

- **Modifier Tags**: Tags that classify this modifier (like "DamageOverTime" or "StatusEffect")
  - Useful for cancelation and querying
</details>

<details markdown="1">
  <summary>Duration Config</summary>

Duration modifiers have additional settings that control their behavior over time:

- **Has Infinite Duration**: If true, the modifier stays active until explicitly removed
- **Duration**: How long the modifier lasts if not infinite (in seconds)
- **Tick On Apply**: Whether to apply the effect immediately upon activation
- **Tick Interval**: How frequently the modifier applies its effect (in seconds)
- **Tick Tag Requirement Behavior**: What happens if tag requirements fail during the effect
  - **Skip**: Skip the current tick but continue the duration
  - **Pause**: Freeze the timer until requirements are met again
  - **Cancel**: End the modifier completely if requirements fail

For example, a "Burning" modifier might last 10 seconds and deal damage every 2 seconds.
</details>

<details markdown="1">
  <summary>Stacking Config</summary>

Duration modifiers can optionally stack when applied multiple times:

- **Can Stack**: Whether applying the same modifier again adds stacks
- **Stacks**: Current number of stacks
- **Has Max Stacks**: Whether there's a cap on stacks
- **Max Stacks**: Maximum number of stacks allowed

The stack count is available to your modifier logic. For example, a burning effect might deal `1 + Stacks` damage per tick.
</details>

<details markdown="1">
  <summary>Requirements</summary>

These settings determine when the modifier can be applied:

- **Target Required Tags**: Tags that must be present on the target
- **Target Blocking Tags**: Tags that prevent application if present
- **Target Blocking Modifier Tags**: Prevents application if another modifier with these tags is active

For example, a "Freezing" effect might require the target to have "Can.Freeze" and be blocked if they have "Status.Immune.Cold".
</details>

<details markdown="1">
  <summary>Application</summary>

These settings control what happens when the modifier is applied:

- **Cancel Abilities**: List of ability classes to cancel when applied
- **Cancel Abilities With Ability Tags**: Tags that identify abilities to cancel
- **Cancel Modifiers With Tag**: Tags that identify other modifiers to cancel
- **Temporarily Applied Tags**: Tags added during the modifier's duration and removed when it ends
- **Permanently Applied Tags**: Tags added that remain even after the modifier ends
- **Remove Gameplay Tags**: Tags to remove from the target when applied
</details>

<details markdown="1">
  <summary>Modifiers</summary>

This is where you define the actual attribute changes. Each modifier entry specifies:

- **Attribute Type**: Float or Struct attribute
- **Modified Attribute**: Tag identifying the attribute to change
- **Modified Attribute Value Type**: Which value to modify (CurrentValue, BaseValue, etc.)
- **Cancel If Attribute Not Found**: Whether to fail if the attribute doesn't exist
- **Application Triggers**: When this modification should apply (e.g., on initial application, on tick, when ending)
- **Modification Operation**: Add, Multiply, or Override
- **Modification Input Value Source**: Where to get the value for the modification

The value source can be:
- **Manual**: A fixed value you specify
- **From Overflow**: Leftover value from a previous modification
- **From Instigator Attribute**: Value from the attribute of whoever applied the modifier
- **From Target Attribute**: Value from one of the target's other attributes
- **From Meta Attribute**: Value calculated by custom blueprint logic

For struct attributes, you specify an operation tag that determines how the struct is modified.
</details>

<details markdown="1">
  <summary>Side Effects</summary>

Side effects are additional actions triggered by the modifier:

- **Ability Side Effects**: Activate other abilities
  - Useful for visual effects or follow-up gameplay mechanics
  
- **Event Side Effects**: Send events through the event system
  - Great for notifying UI or other systems
  
- **Attribute Modifier Side Effects**: Apply other modifiers
  - Allows for chaining effects together

Each side effect can be configured to trigger at specific phases:
- On initial application
- Each time a duration modifier ticks
- When the modifier ends successfully
- If the modifier is cancelled

For example, a "Burning" effect might spawn fire particles, make the character shout in pain, and apply a movement slow.
</details>

<details markdown="1">
  <summary>Blueprint Override Functions</summary>

You can extend Attribute Modifiers in Blueprint by overriding these functions:

- **CanApplyModifier**: Determine if the modifier can be applied
  - Return true/false based on custom conditions
  
- **OnPreApplyModifier**: Called just before applying the modifier
  - Use for setup work
  
- **OnPostApplyModifier**: Called after the modifier is applied
  - Use for follow-up actions
  
- **OnModifierEnded**: Called when the modifier ends (success or cancel)
  - Use for cleanup
  
- **OnStacksAdded**: Called when stacks are added to a duration modifier
  - Handle stack-specific logic
  
- **OnMaxStacksReached**: Called when maximum stacks are reached
  - Special behavior at max stacks
  
- **GetFloatMetaAttributeValue**: Calculate custom values for modifiers
  - Used with the "FromMetaAttribute" input source
  
- **GetModifiedStructAttributeValue**: Custom logic for struct modifications
  - Define how struct attributes change
</details>

<details markdown="1">
  <summary>Usage Examples</summary>

Here are some examples of how you might use Attribute Modifiers:

- **Direct Damage**: An instant modifier that reduces health
  ```
  Health = Health - 20
  ```

- **Damage Over Time**: A duration modifier that reduces health each tick
  ```
  Duration: 5 seconds
  Tick Interval: 1 second
  Health = Health - 5 (each tick)
  ```

- **Buff**: A duration modifier that increases a stat temporarily
  ```
  Duration: 30 seconds
  MovementSpeed = MovementSpeed * 1.5
  ```

- **Chain Effect**: A modifier that applies another effect when it ends
  ```
  Burning effect with Chilled effect as a side effect when cancelled
  ```

- **Resource Cost**: An instant modifier that consumes a resource
  ```
  Mana = Mana - SpellCost
  ```

- **Status Effect**: A complex modifier with multiple effects and visual changes
  ```
  Apply "Stunned" tag
  Cancel movement abilities
  Spawn stun particle effect (ability side effect)
  ```
</details>

<details markdown="1">
  <summary>Tips for Working with Modifiers</summary>

- **Start Simple**: Create basic modifiers before attempting complex ones
- **Use Consistent Tags**: Develop a clear tag hierarchy for your requirements
- **Test Edge Cases**: Make sure modifiers behave correctly when stacked or cancelled
- **Consider Multiplayer**: Be thoughtful about replication policies for networked games
- **Chain Modifiers**: Use side effects to create complex, interconnected systems
- **Document Your Modifiers**: Especially for team projects, document what each modifier does
</details>

Attribute Modifiers are where SimpleGAS really shines - they give you the tools to create rich, interactive gameplay systems that are easy to understand and extend.