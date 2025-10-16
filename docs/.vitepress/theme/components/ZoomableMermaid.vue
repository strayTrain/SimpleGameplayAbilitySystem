<template>
  <div class="zoomable-mermaid-wrapper">
    <div class="zoom-controls">
      <button @click="zoomIn" class="zoom-btn" title="Zoom In">
        <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <circle cx="11" cy="11" r="8"></circle>
          <line x1="11" y1="8" x2="11" y2="14"></line>
          <line x1="8" y1="11" x2="14" y2="11"></line>
          <line x1="21" y1="21" x2="16.65" y2="16.65"></line>
        </svg>
      </button>
      <button @click="zoomOut" class="zoom-btn" title="Zoom Out">
        <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <circle cx="11" cy="11" r="8"></circle>
          <line x1="8" y1="11" x2="14" y2="11"></line>
          <line x1="21" y1="21" x2="16.65" y2="16.65"></line>
        </svg>
      </button>
      <button @click="resetZoom" class="zoom-btn" title="Reset View">
        <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"></path>
          <path d="M21 3v5h-5"></path>
          <path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"></path>
          <path d="M3 21v-5h5"></path>
        </svg>
      </button>
      <span class="zoom-level">{{ Math.round(scale * 100) }}%</span>
    </div>
    <div
      ref="container"
      class="mermaid-container"
      @mousedown="startPan"
      @mousemove="doPan"
      @mouseup="endPan"
      @mouseleave="endPan"
      @wheel.prevent="handleWheel"
      @touchstart="handleTouchStart"
      @touchmove="handleTouchMove"
      @touchend="handleTouchEnd"
    >
      <div
        ref="content"
        class="mermaid-content"
        :style="{ transform: `translate(${translateX}px, ${translateY}px) scale(${scale})` }"
      >
        <pre ref="mermaidDiv" class="mermaid">{{ props.code }}</pre>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, nextTick } from 'vue'
import mermaid from 'mermaid'

const props = defineProps({
  code: {
    type: String,
    required: true
  }
})

const container = ref(null)
const content = ref(null)
const mermaidDiv = ref(null)
const scale = ref(1)
const translateX = ref(0)
const translateY = ref(0)
const isPanning = ref(false)
const startX = ref(0)
const startY = ref(0)
const lastX = ref(0)
const lastY = ref(0)

// Touch handling
const lastTouchDistance = ref(0)
const lastTouchCenter = ref({ x: 0, y: 0 })

const zoomIn = () => {
  const newScale = Math.min(scale.value + 0.2, 10)
  scale.value = newScale
}

const zoomOut = () => {
  const newScale = Math.max(scale.value - 0.2, 0.5)
  scale.value = newScale
}

const resetZoom = () => {
  scale.value = 1
  translateX.value = 0
  translateY.value = 0
}

const startPan = (e) => {
  if (e.button !== 0) return // Only left mouse button
  isPanning.value = true
  startX.value = e.clientX - translateX.value
  startY.value = e.clientY - translateY.value
  container.value.style.cursor = 'grabbing'
}

const doPan = (e) => {
  if (!isPanning.value) return
  translateX.value = e.clientX - startX.value
  translateY.value = e.clientY - startY.value
}

const endPan = () => {
  isPanning.value = false
  if (container.value) {
    container.value.style.cursor = 'grab'
  }
}

const handleWheel = (e) => {
  const delta = e.deltaY > 0 ? -0.1 : 0.1
  const newScale = Math.max(0.5, Math.min(10, scale.value + delta))

  // Zoom towards mouse cursor
  const rect = container.value.getBoundingClientRect()
  const mouseX = e.clientX - rect.left
  const mouseY = e.clientY - rect.top

  const scaleDiff = newScale - scale.value
  translateX.value -= (mouseX - translateX.value) * scaleDiff / scale.value
  translateY.value -= (mouseY - translateY.value) * scaleDiff / scale.value

  scale.value = newScale
}

// Touch event handlers
const getTouchDistance = (touches) => {
  const dx = touches[0].clientX - touches[1].clientX
  const dy = touches[0].clientY - touches[1].clientY
  return Math.sqrt(dx * dx + dy * dy)
}

const getTouchCenter = (touches) => {
  return {
    x: (touches[0].clientX + touches[1].clientX) / 2,
    y: (touches[0].clientY + touches[1].clientY) / 2
  }
}

const handleTouchStart = (e) => {
  if (e.touches.length === 1) {
    // Single touch - pan
    isPanning.value = true
    lastX.value = e.touches[0].clientX
    lastY.value = e.touches[0].clientY
  } else if (e.touches.length === 2) {
    // Two touches - zoom
    isPanning.value = false
    lastTouchDistance.value = getTouchDistance(e.touches)
    lastTouchCenter.value = getTouchCenter(e.touches)
  }
}

const handleTouchMove = (e) => {
  e.preventDefault()

  if (e.touches.length === 1 && isPanning.value) {
    // Pan
    const deltaX = e.touches[0].clientX - lastX.value
    const deltaY = e.touches[0].clientY - lastY.value
    translateX.value += deltaX
    translateY.value += deltaY
    lastX.value = e.touches[0].clientX
    lastY.value = e.touches[0].clientY
  } else if (e.touches.length === 2) {
    // Pinch zoom
    const distance = getTouchDistance(e.touches)
    const center = getTouchCenter(e.touches)

    if (lastTouchDistance.value > 0) {
      const scaleDelta = distance / lastTouchDistance.value
      const newScale = Math.max(0.5, Math.min(10, scale.value * scaleDelta))

      // Zoom towards pinch center
      const rect = container.value.getBoundingClientRect()
      const centerX = center.x - rect.left
      const centerY = center.y - rect.top

      const scaleDiff = newScale - scale.value
      translateX.value -= (centerX - translateX.value) * scaleDiff / scale.value
      translateY.value -= (centerY - translateY.value) * scaleDiff / scale.value

      scale.value = newScale
    }

    lastTouchDistance.value = distance
    lastTouchCenter.value = center
  }
}

const handleTouchEnd = (e) => {
  if (e.touches.length === 0) {
    isPanning.value = false
    lastTouchDistance.value = 0
  } else if (e.touches.length === 1) {
    lastX.value = e.touches[0].clientX
    lastY.value = e.touches[0].clientY
    lastTouchDistance.value = 0
  }
}

onMounted(async () => {
  mermaid.initialize({
    startOnLoad: true,
    theme: 'default',
    securityLevel: 'loose',
  })

  await nextTick()

  if (mermaidDiv.value && props.code) {
    try {
      await mermaid.run({
        nodes: [mermaidDiv.value]
      })
    } catch (error) {
      console.error('Mermaid rendering error:', error)
    }
  }

  if (container.value) {
    container.value.style.cursor = 'grab'
  }
})
</script>

<style scoped>
.zoomable-mermaid-wrapper {
  position: relative;
  width: 100%;
  margin: 2rem 0;
}

.zoom-controls {
  position: absolute;
  top: 1rem;
  right: 1rem;
  display: flex;
  gap: 0.5rem;
  z-index: 10;
  background: var(--vp-c-bg);
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
  padding: 0.5rem;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.zoom-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  background: var(--vp-c-bg-soft);
  border: 1px solid var(--vp-c-divider);
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s ease;
  color: var(--vp-c-text-1);
}

.zoom-btn:hover {
  background: var(--vp-c-brand-soft);
  border-color: var(--vp-c-brand);
  color: var(--vp-c-brand);
}

.zoom-btn:active {
  transform: scale(0.95);
}

.zoom-level {
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 50px;
  padding: 0 0.5rem;
  font-size: 0.875rem;
  font-weight: 500;
  color: var(--vp-c-text-2);
}

.mermaid-container {
  width: 100%;
  height: 600px;
  overflow: hidden;
  border: 1px solid var(--vp-c-divider);
  border-radius: 12px;
  background: var(--vp-c-bg-soft);
  position: relative;
  touch-action: none;
}

.mermaid-content {
  transform-origin: 0 0;
  transition: transform 0.05s ease-out;
  will-change: transform;
  width: fit-content;
  height: fit-content;
  padding: 2rem;
}

.mermaid {
  display: inline-block;
}

/* Make mermaid diagrams visible */
.mermaid-content :deep(svg) {
  max-width: none !important;
  height: auto !important;
}

/* Improve readability of mermaid text */
.mermaid-content :deep(.nodeLabel),
.mermaid-content :deep(.edgeLabel) {
  font-size: 14px;
}

/* Mobile responsive */
@media (max-width: 768px) {
  .zoom-controls {
    top: 0.5rem;
    right: 0.5rem;
    padding: 0.25rem;
    gap: 0.25rem;
  }

  .zoom-btn {
    width: 32px;
    height: 32px;
  }

  .zoom-level {
    font-size: 0.75rem;
    min-width: 45px;
  }

  .mermaid-container {
    height: 400px;
  }
}
</style>
