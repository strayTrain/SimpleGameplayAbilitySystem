<template>
  <div class="blueprint-embed">
    <iframe
      :src="blueprintUrl"
      scrolling="no"
      allowfullscreen
    ></iframe>
  </div>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  id: String
})

const blueprintUrl = computed(() => `https://blueprintue.com/render/${props.id}/`)
</script>

<style scoped>
.blueprint-embed {
  margin: 2rem 0;
}
.blueprint-embed iframe {
  width: 100%;
  height: 600px;
  border: 1px solid var(--vp-c-divider);
  border-radius: 8px;
}
</style>
