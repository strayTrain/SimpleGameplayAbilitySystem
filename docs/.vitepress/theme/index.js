import DefaultTheme from 'vitepress/theme'
import ImageSlideshow from './components/ImageSlideshow.vue'
import BlueprintEmbed from './components/BlueprintEmbed.vue'
import StepByStep from './components/StepByStep.vue'
import NetworkAnimation from './components/NetworkAnimation.vue'
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
  }
}
