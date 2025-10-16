import DefaultTheme from 'vitepress/theme'
import ImageSlideshow from './components/ImageSlideshow.vue'
import BlueprintEmbed from './components/BlueprintEmbed.vue'
import StepByStep from './components/StepByStep.vue'
import NetworkAnimation from './components/NetworkAnimation.vue'
import InteractiveFeatures from './components/InteractiveFeatures.vue'
import ZoomableMermaid from './components/ZoomableMermaid.vue'
import MarkdownActions from './components/MarkdownActions.vue'
import PluginDescriptionActions from './components/PluginDescriptionActions.vue'
import './custom.css'
import { h } from 'vue'

export default {
  extends: DefaultTheme,
  Layout() {
    return h(DefaultTheme.Layout, null, {
      'home-hero-before': () => h(NetworkAnimation)
    })
  },
  enhanceApp({ app }) {
    app.component('ImageSlideshow', ImageSlideshow)
    app.component('BlueprintEmbed', BlueprintEmbed)
    app.component('StepByStep', StepByStep)
    app.component('NetworkAnimation', NetworkAnimation)
    app.component('InteractiveFeatures', InteractiveFeatures)
    app.component('ZoomableMermaid', ZoomableMermaid)
    app.component('MarkdownActions', MarkdownActions)
    app.component('PluginDescriptionActions', PluginDescriptionActions)
  }
}
