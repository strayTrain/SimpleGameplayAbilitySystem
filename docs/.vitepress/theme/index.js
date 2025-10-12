import DefaultTheme from 'vitepress/theme'
import ImageSlideshow from './components/ImageSlideshow.vue'
import BlueprintEmbed from './components/BlueprintEmbed.vue'
import StepByStep from './components/StepByStep.vue'
import './custom.css'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('ImageSlideshow', ImageSlideshow)
    app.component('BlueprintEmbed', BlueprintEmbed)
    app.component('StepByStep', StepByStep)
  }
}
