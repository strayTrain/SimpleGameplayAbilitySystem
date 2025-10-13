import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'Simple Gameplay Ability System',
  description: 'Documentation for SGAS Unreal Engine Plugin',
  base: '/SimpleGameplayAbilitySystem/',

  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Guide', link: '/guide/setup' },
      { text: 'API', link: '/api/simple-gameplay-ability' },
      { text: 'GitHub Repo', link: 'https://github.com/strayTrain/SimpleGameplayAbilitySystem' }
    ],

    sidebar: {
      '/guide/': [
        {
          text: 'Getting Started',
          items: [
            { text: 'Installing The Plugin', link: '/guide/setup' },
            { text: 'Quick Start', link: '/guide/quick-start' }
          ]
        },
        {
          text: 'Understanding SimpleGAS',
          items: [
            { text: 'High Level Overview', link: '/guide/overview' },
            { text: 'Core Concepts', link: '/guide/concepts' },
            { text: 'Networking', link: '/guide/networking' }
          ]
        }
      ],
      '/api/': [
        {
          text: 'API Reference',
          items: [
            { text: 'SimpleGameplayAbility', link: '/api/simple-gameplay-ability' },
            { text: 'SimpleGameplayAbilityComponent', link: '/api/simple-gameplay-ability-component' },
            { text: 'SimpleAttributeModifier', link: '/api/simple-attribute-modifier' }
          ]
        }
      ]
    },
  }
})
