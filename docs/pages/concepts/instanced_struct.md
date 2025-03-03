---
title: Instanced Structs
layout: home
parent: Concepts
nav_order: 2
---

# Instanced Structs (`FInstancedStruct`)

## Overview

`FInstancedStruct` is a powerful Unreal Engine feature that allows developers to store, manipulate, and transfer struct-based data dynamically at runtime. SimpleGAS makes extensive use of `FInstancedStruct` to provide flexible and extendable gameplay systems, particularly for handling payloads, ability contexts, attribute modifications, and event payloads.

## Why Use `FInstancedStruct`?

### 1. **Runtime Flexibility**
Unlike normal `UStructs`, which require static definitions, `FInstancedStruct` enables storing arbitrary struct types at runtime. This makes it ideal for handling dynamic gameplay data, such as ability payloads or modifier contexts.

### 2. **Type Safety & Reflection Support**
`FInstancedStruct` provides type safety while allowing introspection using Unreal's reflection system. This is crucial for debugging, serialization, and replication.

### 3. **Efficient Memory Usage**
By storing only the necessary data and allowing type-less manipulation, `FInstancedStruct` reduces the need for unnecessary class inheritance, reducing memory overhead.

## How `FInstancedStruct` is Used in SimpleGAS

### Ability Activation Payloads
In SimpleGAS, abilities can be activated with a payload that is passed as an `FInstancedStruct`. This allows developers to define and send arbitrary context data when activating an ability.

```cpp
FGuid AbilityID = AbilityComponent->ActivateAbility(AbilityClass, FInstancedStruct::Make(MyCustomPayload), false, EAbilityActivationPolicy::LocalOnly);
```

### Attribute Modifiers
`FInstancedStruct` is used to store custom modifier contexts, allowing each modifier to carry different types of data without needing a rigid, predefined structure.

```cpp
FInstancedStruct ModifierContext = FInstancedStruct::Make(MyModifierData);
MyAttributeModifier->ApplyModifier(InstigatorComponent, TargetComponent, ModifierContext);
```

### Event Payloads
Events in the SimpleEventSubsystem can carry an `FInstancedStruct` payload, enabling arbitrary data to be sent and received dynamically.

```cpp
FInstancedStruct Payload = FInstancedStruct::Make(MyEventData);
EventSubsystem->SendEvent(MyEventTag, MyDomainTag, Payload, Sender);
```

## Working with `FInstancedStruct`

### Creating an `FInstancedStruct`
To store custom data in an `FInstancedStruct`, initialize it with a concrete struct type:

```cpp
FInstancedStruct MyStruct;
MyStruct.InitializeAs<FMyCustomStruct>();
FMyCustomStruct& Data = MyStruct.GetMutable<FMyCustomStruct>();
Data.MyValue = 42;
```

Alternatively, if you have an existing struct:

```cpp
FInstancedStruct MyStruct = FInstancedStruct::Make(FMyCustomStruct{42});
```

### Retrieving Data from `FInstancedStruct`
To extract the stored struct:

```cpp
if (MyStruct.IsValid())
{
    const FMyCustomStruct* RetrievedData = MyStruct.GetPtr<FMyCustomStruct>();
    if (RetrievedData)
    {
        UE_LOG(LogTemp, Log, TEXT("Value: %d"), RetrievedData->MyValue);
    }
}
```

For mutable access:

```cpp
FMyCustomStruct* RetrievedData = MyStruct.GetMutable<FMyCustomStruct>();
if (RetrievedData)
{
    RetrievedData->MyValue = 99;
}
```

### Checking if `FInstancedStruct` is Valid
Before using an `FInstancedStruct`, ensure it contains valid data:

```cpp
if (MyStruct.IsValid())
{
    // Safe to use
}
```
