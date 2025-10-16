---
layout: home

hero:
  text: Simple Gameplay Ability System
  tagline: A multiplayer framework for Unreal Engine
  actions:
    - theme: brand
      text: Documentation
      link: /docs/problem_overview
    - theme: alt
      text: View on GitHub
      link: https://github.com/strayTrain/SimpleGameplayAbilitySystem
---

<InteractiveFeatures :features="[
  {
    icon: '🌐',
    title: 'Simplify Multiplayer Boilerplate',
    summary: 'Eliminates tedious multiplayer setup and replication code',
    details: `
      <p>Simple Gameplay Ability System handles the complex multiplayer infrastructure for you:</p>
      <ul>
        <li><strong>Automatic Replication:</strong> Abilities and attributes replicate seamlessly without manual RPC setup</li>
        <li><strong>Client-Server Architecture:</strong> Built-in prediction and reconciliation for smooth gameplay</li>
        <li><strong>No Boilerplate:</strong> Unlike Epic's GAS, SGAS minimizes setup code and configuration</li>
        <li><strong>Network Optimized:</strong> Efficient bandwidth usage with optimized replication</li>
        <li><strong>Authority Handling:</strong> Automatic server authority validation built-in</li>
        <li><strong>Quick Setup:</strong> Add the component and start building multiplayer features immediately</li>
      </ul>
      <p>Focus on gameplay, not networking code.</p>
    `
  },
  {
    icon: '🧩',
    title: 'Modular and Data Driven',
    summary: 'Flexible architecture that adapts to your project',
    details: `
      <p>Built with modularity and data-driven design at its core:</p>
      <ul>
        <li><strong>Modular Components:</strong> Use only the features you need without bloat</li>
        <li><strong>Data Assets:</strong> Configure abilities through data assets that designers can modify</li>
        <li><strong>Attribute Sets:</strong> Define custom attributes for health, mana, stamina, and more</li>
        <li><strong>Tag-Based System:</strong> Flexible gameplay tag system for ability requirements and effects</li>
        <li><strong>Composable Effects:</strong> Build complex behaviors by combining simple, reusable effects</li>
        <li><strong>Easy Extension:</strong> Extend base classes to create project-specific functionality</li>
      </ul>
      <p>Scales from small prototypes to large production games.</p>
    `
  },
  {
    icon: '🎨',
    title: 'Blueprint Friendly',
    summary: 'Full blueprint support for designers',
    details: `
      <p>Empowers designers and programmers alike with comprehensive Blueprint support:</p>
      <ul>
        <li><strong>Create Abilities in Blueprints:</strong> Full Blueprint support for creating custom abilities without C++ code</li>
        <li><strong>Visual Scripting:</strong> All major functions exposed to Blueprints with clear, descriptive names</li>
        <li><strong>Designer-Friendly:</strong> Non-programmers can create complex ability behaviors</li>
        <li><strong>Event-Driven:</strong> Blueprint events for ability activation, cancellation, and completion</li>
        <li><strong>Extensible:</strong> Mix Blueprint and C++ abilities in the same project seamlessly</li>
        <li><strong>Rapid Iteration:</strong> Test and modify abilities without recompiling C++ code</li>
      </ul>
      <p>Enables rapid iteration and empowers your entire team to create engaging gameplay.</p>
    `
  },
  {
    icon: '🫸✨🫷',
    title: 'Free and Open Source',
    summary: 'MIT licensed and community driven',
    details: `
      <p>Completely free to use with full source code access:</p>
      <ul>
        <li><strong>MIT License:</strong> Use in any project, commercial or personal, with no restrictions</li>
        <li><strong>Full Source Access:</strong> Complete C++ source code included for transparency and customization</li>
        <li><strong>Community Driven:</strong> Open to contributions and feedback from the community</li>
        <li><strong>No Vendor Lock-in:</strong> You own your code and can modify the plugin however you need</li>
        <li><strong>Active Development:</strong> Regularly updated with bug fixes and new features</li>
        <li><strong>GitHub Hosted:</strong> Easy to fork, contribute, and track issues</li>
      </ul>
      <p>Built by developers, for developers.</p>
    `
  }
]" />
