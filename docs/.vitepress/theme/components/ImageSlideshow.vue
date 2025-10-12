<template>
  <div class="slideshow">
    <div class="slideshow-container">
      <img :src="images[currentIndex]" :alt="`Slide ${currentIndex + 1}`" />
      <button @click="prev" class="prev">❮</button>
      <button @click="next" class="next">❯</button>
    </div>
    <div class="dots">
      <span
        v-for="(_, index) in images"
        :key="index"
        :class="['dot', { active: index === currentIndex }]"
        @click="currentIndex = index"
      ></span>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const props = defineProps({
  images: Array
})

const currentIndex = ref(0)

const next = () => {
  currentIndex.value = (currentIndex.value + 1) % props.images.length
}

const prev = () => {
  currentIndex.value = (currentIndex.value - 1 + props.images.length) % props.images.length
}
</script>

<style scoped>
.slideshow {
  margin: 2rem 0;
}
.slideshow-container {
  position: relative;
  max-width: 800px;
  margin: auto;
}
.slideshow-container img {
  width: 100%;
  border-radius: 8px;
}
.prev, .next {
  position: absolute;
  top: 50%;
  transform: translateY(-50%);
  background: rgba(0,0,0,0.5);
  color: white;
  border: none;
  padding: 16px;
  cursor: pointer;
  font-size: 18px;
  border-radius: 4px;
}
.prev { left: 10px; }
.next { right: 10px; }
.dots {
  text-align: center;
  padding: 20px;
}
.dot {
  height: 12px;
  width: 12px;
  margin: 0 4px;
  background-color: #bbb;
  border-radius: 50%;
  display: inline-block;
  cursor: pointer;
}
.dot.active {
  background-color: #717171;
}
</style>
