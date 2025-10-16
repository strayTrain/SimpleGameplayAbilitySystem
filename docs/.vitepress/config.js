import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(defineConfig({
  title: 'Simple GAS',
  description: 'Documentation for SGAS Unreal Engine Plugin',
  base: '/SimpleGameplayAbilitySystem/',

  mermaid: {
    // Optional: Mermaid configuration
  },

  mermaidPlugin: {
    class: 'mermaid'
  },

  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Docs', link: '/docs/problem_overview' },
      { text: 'GitHub Repo', link: 'https://github.com/strayTrain/SimpleGameplayAbilitySystem' }
    ],

    sidebar: {
      '/docs/': [
        {
          text: 'Understanding SimpleGAS',
          items: [
            { text: 'Why Is It Useful?', link: '/docs/problem_overview' },
            { text: 'High Level Overview', link: '/docs/overview' },
            { text: 'Replication Overview', link: '/docs/networking' }
          ]
        },
        {
          text: 'Getting Started',
          items: [
            { text: 'Installing The Plugin', link: '/docs/setup' },
            { text: 'Quick Start', link: '/docs/quick-start' }
          ]
        },
        {
          text: 'API Reference',
          items: [
            { text: 'SimpleGameplayAbility', link: '/docs/simple-gameplay-ability' },
            { text: 'SimpleGameplayAbilityComponent', link: '/docs/simple-gameplay-ability-component' },
            { text: 'SimpleAttributeComponent', link: '/docs/simple-attribute-component' },
            { text: 'SimpleAttributeModifier', link: '/docs/simple-attribute-modifier' }
          ]
        },
        {
          text: 'Resources',
          items: [
            { text: 'Reference for AI Models', link: '/docs/plugin_description' }
          ]
        }
      ]
    },
  }
}))
