---
title: Detailed Walkthrough
layout: home
nav_order: 4
---

# Our Goal

Let's implement the Kick ability example in full from the [High Level Overview](high_level_overview.html) using SimpleGAS.  

To reiterate the what the ability does:
1. The ability is client predicted
2. We do a spinning kick
3. At the high point of the kick we do an overlap check in a sphere to see if we hit anything and apply a damage modifier to the hit actors
    * The damage modifier deals 20 damage to the hit actors. They have 10 armour and 100 health. We expect the damage to first remove armour and then health. So the final result should be 0 armour and 90 health.
4. We take a snapshot of all the actors we hit and the modifier we applied to them
5. If we hit something on the client but it didn't hit on the server, we cancel the modifier we applied locally (which will also cancel the hit reaction animation)

<video width="640" height="360" controls>
<source src="../videos/kick_result.mp4" type="video/mp4">
Your browser does not support the video tag.
</video>