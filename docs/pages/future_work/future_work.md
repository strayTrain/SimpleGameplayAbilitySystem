---
title: Future Work
layout: home
nav_order: 5
---

---

*A note from the dev:*  

I'm currently working on a personal project that uses SimpleGAS and I'm fixing bugs/adding features as I come across them.  
There are many ways to make a game and I couldn't possibly test every feature of the plugin with a single project. So, if you have a specific feature that you would like to see added/improved/fixed, please open an issue on the [GitHub repository](https://github.com/strayTrain/SimpleGameplayAbilitySystem/issues) or start a discussion in the [discussions page](https://github.com/strayTrain/SimpleGameplayAbilitySystem/discussions).

---

That said, here's a list of things I plan to work on in the future:
- [ ] **Documentation**:  
    - Add examples to the documentation for various features
    - Create a demo project that showcases the plugin's features
- [ ] **Features**:  
    - Add a way to set an ability's cooldown dynamically
    - Add variants to common functions (e.g. a version of `ActivateAbility` that takes multiple contexts)
    - Create a more reliable network clock 
    - Add a way for the amount of replicated data to be configurable (currently all attributes are replicated)
    - Add a way to set the replication frequency of attributes
    - Improve performance of various systems (there is always room for improvement)
- [ ] **Events**:  
    - Add more built-in events that are broadcasted by the system e.g. Adding/Removing abilities etc.
