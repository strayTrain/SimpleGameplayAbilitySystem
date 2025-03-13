---
title: Creating a replicated ability
layout: home
parent: Examples
nav_order: 2
---

# Creating a replicated Kick ability

In this example we're going to
- Create an ability that makes the player play a kick animation and launches any hit players away from the player.
- Set the ability up so that it both replicates and is client predicted.

<video width="320" height="240" controls>
    <source src="/videos/LaunchPlayerResult.mp4" type="video/mp4">
    Your browser does not support the video tag.
</video>



<details markdown="1">
<summary>Tips</summary>

- Currently, we can activate the ability repeatedly if we keep pressing the F key. We can fix this by overriding `CanActivate` on `GA_LaunchPlayer` and returning false if the player is not on the ground.  
    <a href="images/basic_setup_8.png" target="_blank">
    ![a screenshot of the CanActivate function of GA_LaunchPlayer](images/basic_setup_8.png)
    </a>
- We can ensure that the ability has the data and references it needs by setting `RequiredContextType` and `AvatarActorFilter` on the `GA_LaunchPlayer` class. This means that the ability won't activate if it wasn't passed the right context struct or if the `AvatarActor` is not the right type.
    <a href="images/basic_setup_9.png" target="_blank">
    ![a screenshot of the RequiredContextType and AvatarActorFilter properties](images/basic_setup_9.png)
    </a>

</details>