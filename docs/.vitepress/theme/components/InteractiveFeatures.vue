<template>
  <div class="interactive-features">
    <div class="features-grid">
      <template v-for="(feature, index) in features" :key="index">
        <div
          :ref="el => cardRefs[index] = el"
          :class="['feature-card', { active: activeIndex === index }]"
          @click="selectFeature(index)"
        >
          <div class="icon">{{ feature.icon }}</div>
          <h3>{{ feature.title }}</h3>
          <p>{{ feature.summary }}</p>
        </div>

        <!-- Mobile: inline details after each card -->
        <transition name="slide-fade">
          <div
            v-if="isMobile && activeIndex === index"
            :ref="el => detailsRef = el"
            class="details-panel details-panel-inline"
          >
            <div class="details-content">
              <h3>{{ feature.title }}</h3>
              <div class="details-body" v-html="feature.details"></div>
            </div>
          </div>
        </transition>
      </template>
    </div>

    <!-- Desktop: unified details panel at bottom -->
    <transition name="slide-fade">
      <div
        v-if="!isMobile && activeIndex !== null"
        :ref="el => detailsRef = el"
        class="details-panel details-panel-bottom"
      >
        <div class="details-content">
          <h3>{{ features[activeIndex].title }}</h3>
          <div class="details-body" v-html="features[activeIndex].details"></div>
        </div>
      </div>
    </transition>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, nextTick } from 'vue'

const props = defineProps({
  features: {
    type: Array,
    required: true
  }
})

const activeIndex = ref(null)
const isMobile = ref(false)
const cardRefs = ref([])
const detailsRef = ref(null)

const checkMobile = () => {
  isMobile.value = window.innerWidth < 768
}

const selectFeature = async (index) => {
  const wasActive = activeIndex.value === index
  activeIndex.value = wasActive ? null : index

  // On mobile, scroll to show the tapped card at the top
  if (!wasActive && isMobile.value && activeIndex.value !== null) {
    await nextTick()
    const tappedCard = cardRefs.value[index]
    if (tappedCard) {
      tappedCard.scrollIntoView({
        behavior: 'smooth',
        block: 'start'
      })
    }
  }
}

onMounted(() => {
  checkMobile()
  window.addEventListener('resize', checkMobile)
})

onUnmounted(() => {
  window.removeEventListener('resize', checkMobile)
})
</script>

<style scoped>
.interactive-features {
  padding: 2rem 0;
}

.features-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 1.5rem;
  margin-bottom: 2rem;
}

/* On mobile, make it single column for better accordion behavior */
@media (max-width: 767px) {
  .features-grid {
    grid-template-columns: 1fr;
    margin-bottom: 0;
  }
}

.feature-card {
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 12px;
  padding: 2rem;
  cursor: pointer;
  transition: all 0.3s ease;
  position: relative;
}

.feature-card:hover {
  border-color: var(--vp-c-brand);
  transform: translateY(-4px);
  box-shadow: 0 8px 16px rgba(0, 0, 0, 0.1);
}

@media (max-width: 767px) {
  .feature-card:hover {
    transform: none;
  }
}

.feature-card.active {
  border-color: var(--vp-c-brand);
  background: var(--vp-c-brand-soft);
}

/* Arrow indicator only on desktop */
@media (min-width: 768px) {
  .feature-card.active::after {
    content: '';
    position: absolute;
    bottom: -12px;
    left: 50%;
    transform: translateX(-50%);
    width: 0;
    height: 0;
    border-left: 12px solid transparent;
    border-right: 12px solid transparent;
    border-bottom: 12px solid var(--vp-c-bg-soft);
  }
}

.icon {
  font-size: 3rem;
  margin-bottom: 1rem;
  text-align: center;
}

.feature-card h3 {
  font-size: 1.25rem;
  font-weight: 600;
  margin-bottom: 0.5rem;
  color: var(--vp-c-text-1);
}

.feature-card p {
  font-size: 0.95rem;
  color: var(--vp-c-text-2);
  margin: 0;
  line-height: 1.6;
}

.details-panel {
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 12px;
  padding: 2rem;
}

/* Mobile inline details - appears right after the card */
.details-panel-inline {
  margin-top: 0;
  border-top: none;
  border-top-left-radius: 0;
  border-top-right-radius: 0;
  margin-bottom: 1.5rem;
}

/* Desktop bottom details - appears at the bottom of all cards */
.details-panel-bottom {
  margin-top: 1rem;
}

.details-content h3 {
  font-size: 1.5rem;
  font-weight: 600;
  margin-bottom: 1rem;
  color: var(--vp-c-brand);
}

.details-body {
  font-size: 1rem;
  line-height: 1.7;
  color: var(--vp-c-text-1);
}

.details-body :deep(ul) {
  padding-left: 1.5rem;
  margin: 1rem 0;
}

.details-body :deep(li) {
  margin: 0.5rem 0;
}

.details-body :deep(code) {
  background: var(--vp-c-bg-mute);
  padding: 0.2rem 0.4rem;
  border-radius: 4px;
  font-family: var(--vp-font-family-mono);
  font-size: 0.9em;
}

.details-body :deep(p) {
  margin: 1rem 0;
}

.slide-fade-enter-active {
  transition: all 0.3s ease-out;
}

.slide-fade-leave-active {
  transition: all 0.2s ease-in;
}

.slide-fade-enter-from {
  transform: translateY(-20px);
  opacity: 0;
}

.slide-fade-leave-to {
  transform: translateY(-10px);
  opacity: 0;
}
</style>
