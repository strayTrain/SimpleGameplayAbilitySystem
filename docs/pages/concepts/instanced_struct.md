---
title: Instanced Structs
layout: home
parent: Concepts
nav_order: 2
---

# Instanced Structs (`FInstancedStruct`)

## What is an Instanced Struct?

An `FInstancedStruct` is a flexible container that can hold any Unreal Engine struct type. Think of it as a box that can store different types of data at runtime, which makes it great for passing around information when you don't know the exact type ahead of time.

SimpleGAS uses `FInstancedStruct` extensively to make its systems more flexible and extendable.

![Instanced struct visualization](TODO)

## Why Do We Need Instanced Structs?

In SimpleGAS, different abilities and systems often need to pass different types of data to each other. For example:

- A fire ability might need to pass burn damage information
- A healing spell might need to pass healing amount and targets
- A buff might need to pass stat multipliers

Rather than creating rigid systems with predefined data structures, `FInstancedStruct` allows us to handle any type of data dynamically.

## How SimpleGAS Uses Instanced Structs

### Ability Activation Context

When you activate an ability, you can pass in any struct as context:

```cpp
// Define a custom context struct
struct FFireballContext
{
    float Damage;
    float Radius;
    bool CanBurnEnvironment;
};

// Activate the ability with this context
FFireballContext Context;
Context.Damage = 50.0f;
Context.Radius = 300.0f;
Context.CanBurnEnvironment = true;

AbilityComponent->ActivateAbility(FireballAbilityClass, 
    FInstancedStruct::Make(Context));
```

Inside the ability, you can retrieve this information:

```cpp
// In your ability's OnActivate function
if (const FFireballContext* Context = 
    ActivationContext.GetPtr<FFireballContext>())
{
    // Use Context->Damage, Context->Radius, etc.
}
```

### Attribute Modifier Context

Attribute modifiers can receive custom contexts to determine how they modify attributes:

```cpp
FDamageContext DamageInfo;
DamageInfo.DamageType = EDamageType::Fire;
DamageInfo.IsCritical = true;
DamageInfo.SourceAbility = AbilityID;

// Apply a damage modifier with this context
AbilityComponent->ApplyAttributeModifierToTarget(
    TargetComponent, 
    DamageModifierClass,
    FInstancedStruct::Make(DamageInfo),
    ModifierID);
```

### Event Payloads

The event system uses `FInstancedStruct` to pass arbitrary data with events:

```cpp
FPlayerDamagedEvent DamageEvent;
DamageEvent.DamageAmount = 50.0f;
DamageEvent.DamageSource = this;
DamageEvent.HitLocation = HitResult.Location;

// Send an event with this payload
AbilityComponent->SendEvent(
    GameplayTags.DamageEvent,
    GameplayTags.DamageDomain,
    FInstancedStruct::Make(DamageEvent),
    Instigator);
```

Listeners can then check for and use events with specific payload types:

```cpp
// A function bound to an event callback
void OnDamageEvent(FGameplayTag EventTag, FGameplayTag DomainTag, 
    FInstancedStruct Payload, AActor* Sender)
{
    if (const FPlayerDamagedEvent* DamageEvent = 
        Payload.GetPtr<FPlayerDamagedEvent>())
    {
        // Process the damage event
        HandleDamage(DamageEvent->DamageAmount, DamageEvent->HitLocation);
    }
}
```

## Working with Instanced Structs

### Creating an Instanced Struct

There are a few ways to create and use `FInstancedStruct`:

```cpp
// Method 1: Make from an existing struct
FMyCustomStruct MyData;
MyData.SomeValue = 42;
FInstancedStruct Container = FInstancedStruct::Make(MyData);

// Method 2: Initialize as a specific type and then set values
FInstancedStruct Container;
Container.InitializeAs<FMyCustomStruct>();
Container.GetMutable<FMyCustomStruct>()->SomeValue = 42;
```

### Reading from an Instanced Struct

To access the data stored in an `FInstancedStruct`:

```cpp
// Method 1: Get a const pointer (safer)
if (const FMyCustomStruct* Data = Container.GetPtr<FMyCustomStruct>())
{
    // Use Data->SomeValue
}

// Method 2: Get a mutable pointer (when you need to modify)
if (FMyCustomStruct* Data = Container.GetMutable<FMyCustomStruct>())
{
    Data->SomeValue = 100;
}

// Method 3: Get a copy of the data
bool WasValid = false;
FMyCustomStruct Data = Container.Get<FMyCustomStruct>(&WasValid);
```

### Checking Validity

Always check if an `FInstancedStruct` contains valid data of the expected type:

```cpp
// Check if the struct contains any valid data
if (Container.IsValid())
{
    // Now check if it contains the expected type
    if (Container.GetScriptStruct() == FMyCustomStruct::StaticStruct())
    {
        // Safe to access as FMyCustomStruct
    }
}
```

## Tips for Using Instanced Structs

- **Type Safety**: Always check the type before casting
- **Keep Structs Simple**: Avoid complex inheritance in structs used with `FInstancedStruct`
- **Documentation**: Document the expected struct types for functions that take or return an `FInstancedStruct`
- **Default Values**: Initialize structs with sensible defaults when possible
- **Error Handling**: Have fallback behavior when an expected struct type isn't found

Instanced structs give SimpleGAS much of its flexibility, allowing for dynamic, data-driven gameplay systems without sacrificing type safety.