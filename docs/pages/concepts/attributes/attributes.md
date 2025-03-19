---
title: Attributes
layout: home
parent: Concepts
nav_order: 4
---

# Attributes

Attributes are values that define stats for your `GameplayAbilitySystemComponent` e.g. health, stamina, speed.  
SimpleGAS gives you two types of attributes to work with: Float Attributes for simple numbers and Struct Attributes for more complex data.

## Float Attributes

- `Attribute Name`: A friendly label to help you identify the attribute in the editor
- `Attribute Tag`: A unique gameplay tag that identifies this attribute
- `Base Value`: A "permanent" value that represents the inherent stat
- `Current Value`: The actual value used during gameplay that can be modified temporarily
    - For example, a character might have a Base Strength of 10, but a Current Strength of 15 due to a temporary buff. When the buff wears off, the Current Strength goes back to the Base Value.
- `Value Limits`: Optional min/max boundaries for the attribute

Tip: You can choose to only use Current Value if that's all you need - the Base Value is there in case you want that extra layer of "permanent vs temporary" changes.

Here's what a Float Attribute looks like in the editor:
<a href="attributes_1.png" target="_blank">
![Float attribute in the editor](attributes_1.png)
</a>

## Struct Attributes

Sometimes a simple number isn't enough. That's where Struct Attributes come in - they can store complex data like inventory items, skill trees, or any custom struct you create.

Struct Attributes have:

- `Attribute Name`: A friendly label to help you identify the attribute in the editor
- `Attribute Tag`: A unique gameplay tag that identifies this attribute
- `StructType`: The struct type this attribute represents
- `StructAttributeHandler`: On optional handler object class reference that you can use to send fine grained events when the attribute changes

Here's an example of a Struct Attribute in the editor:
<a href="attributes_2.png" target="_blank">
![Struct attribute in the editor](attributes_2.png)
</a>

## How To Modify Attributes

<details markdown="1">
<summary>Float Attributes</summary>

1. Calling the [`SetFloatAttribute`](../../blueprint_nodes/ability_component/gameplay_ability_component.html#setfloatattributevalue) function on the `GamplayAbilitySystemComponent` that owns the attribute:  
    <a href="attributes_3.png" target="_blank">
![a screenshot of the SetFloatAttributeValue function](attributes_3.png)
</a>

2. Using an [`Attribute Modifier`](../../blueprint_nodes/attribute_modifiers/attribute_modifiers.html):  
    <a href="attributes_11.png" target="_blank">
![a screenshot of a simple attribute modifier changing a float](attributes_11.png)
</a>  

</details>

<details markdown="1">
<summary>Struct Attributes</summary>

1. Calling the [`SetStructAttribute`](../../blueprint_nodes/ability_component/gameplay_ability_component.html#setstructattributevalue) function on the `GamplayAbilitySystemComponent` that owns the attribute:  
    <a href="attributes_12.png" target="_blank">
![a screenshot of the SetStructAttributeValue function](attributes_12.png)
</a>

2. Using an [`Attribute Modifier`](../../blueprint_nodes/attribute_modifiers/attribute_modifiers.html):  
    <a href="attributes_13.png" target="_blank">
![a screenshot of a simple attribute modifier changing a struct](attributes_13.png)
</a>  
    Inside the struct modification function:  
    <a href="attributes_14.png" target="_blank">
![a screenshot of a function callback when modifying a struct attribute with an attribute modifier](attributes_14.png)
</a>

</details>

## How To Listen For Changes

When an attribute changes, SimpleGAS automatically sends an event through the [Simple Event Subsystem](../event_system/event_subsystem.html) so other systems (like your UI) can react to the change.  
You can find a list of the available events on the [Event Reference page](../../event_reference/event_reference.html). 

<details markdown="1">
<summary>Float Attributes</summary>

1. Listen for the appropriate event in the Simple Event Subsystem:
    <a href="attributes_6.png" target="_blank">
![a screenshot of listening for a change in float attribute value](attributes_6.png)
</a>
2. Use the `WaitForFloatAttributeChanged` latent node:
    <a href="attributes_7.png" target="_blank">
![a screenshot of the WaitForFloatAttributeChanged node](attributes_7.png)
</a>

</details>

<details markdown="1">
<summary>Struct Attributes</summary>

1. Listen for the appropriate event in the Simple Event Subsystem:
    <a href="attributes_8.png" target="_blank">
![a screenshot of listening for a change in struct attribute value](attributes_8.png)
</a>
2. Use the `WaitForStructAttributeChanged` latent node:
    <a href="attributes_9.png" target="_blank">
![a screenshot of the WaitForStructAttributeChanged node](attributes_9.png)
</a>

</details>

### Struct Attribute Handlers

Struct attributes are a bit different from float attributes when it comes to the event that gets sent.  

When a struct attribute changes, the entire struct is replaced. This presents a problem for when we are modifying the struct. 
Even though we know which struct members changed there is no easy way to automatically send an event for each member that changed.  
To get around this, you can create a `StructAttributeHandler` class to send fine-grained events when the attribute changes.  

To create a struct attribute handler, create a new blueprint class that inherits from `UStructAttributeHandler`. This class only has a single function to implement called `GetModificationEvents`. It takes as input two `FInstancedStruct` parameters (the old struct and the new one) and returns an `FGameplayTagContainer` with the tags representing the members of the struct that changed:
<a href="attributes_10.png" target="_blank">
![a screenshot of a struct attribute handler implementation](attributes_10.png)
</a>  
The `GetModificationEvents` function will be  called whenever the struct attribute changes and you can use it to determine which fields in the struct changed.  
You can then listen for the corresponding struct member events. If no `StructAttributeHandler` is set on the struct attribute definition, you'll still receive an event when the struct changes, but it won't have any `ModificationTags` in it.

## Attribute Sets

If you find yourself creating the same attributes for different characters, Attribute Sets can help. They're reusable collections of attributes that you can add to any ability component.

To create an Attribute Set:
1. Right-click in the content browser
2. Choose Create → Data Asset
3. Select "Attribute Set" as the class
4. Add your attributes to the set

Now you can add this set to any ability component instead of manually recreating the same attributes.

Here's what the attribute set looks like in the editor:  
<a href="attributes_4.png" target="_blank">
![a screenshot of an attribute set](attributes_4.png)
</a>

Adding an attribute set to an ability component:  
<a href="attributes_5.png" target="_blank">
![a screenshot of an attribute set reference on an ability component](attributes_5.png)
</a>