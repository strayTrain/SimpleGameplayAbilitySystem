import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'Simple Gameplay Ability System',
  description: 'Documentation for SGAS Unreal Engine Plugin',
  base: '/SimpleGameplayAbilitySystem/',

  themeConfig: {
    nav: [
      { text: 'Home', link: '/' },
      { text: 'Guide', link: '/guide/getting-started' },
      { text: 'API', link: '/api/' },
      { text: 'GitHub', link: 'https://github.com/strayTrain/SimpleGameplayAbilitySystem' }
    ],

    sidebar: {
      '/guide/': [
        {
          text: 'Introduction',
          items: [
            { text: 'Getting Started', link: '/guide/getting-started' },
            { text: 'Installation', link: '/guide/installation' }
          ]
        }
      ]
    },

    socialLinks: [
      { icon: 'github', link: 'https://github.com/strayTrain/SimpleGameplayAbilitySystem' }
    ]
  }
})
