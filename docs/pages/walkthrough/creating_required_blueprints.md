---
title: Setup
layout: home
parent: Detailed Walkthrough
nav_order: 1
---

# Create required blueprints

To start, we need to create the following blueprints:

<details markdown="1">
  <summary>A SimpleGameplayAbility subclass for the kick ability</summary>

I called mine `GA_Kick`.  
![a screenshot of the class creation dialogue in unreal engine](images/BS_GA_Class_Creation.png)

</details>


<details markdown="1">
  <summary>A SimpleAttributeModifier subclass for the damage modifier</summary>

I called mine `AM_KickDamage`.  
![a screenshot of the class creation dialogue in unreal engine for simple gameplay ability](images/BS_AM_Class_Creation.png)

</details>

# Setup the SimpleAbilityComponent

<details markdown="1">
  <summary>Add a SimpleAbilityComponent to your player pawn</summary>

![a screenshot of adding a SimpleGameplayAbilityComponent](images/BS_AC_Adding.png)

</details>

<details markdown="1">
  <summary>Set the Avatar Actor</summary>

* An `Avatar Actor` is the actor that is performing the abilities. 
    * This is usually the player pawn.
    * This allows you to add the ability component to a player controller and upon spawning the pawn, changing the avatar actor. i.e. you can create and destroy the avatar while not losing the attribute data.
    * In our example however the avatar actor and the ability component holder are the same actor for brevity.
    * The `SetAvatarActor` function can only be called on the server.
    ![a screenshot of the SetAvatarActor function](images/BS_AvatarActor.png)

</details>