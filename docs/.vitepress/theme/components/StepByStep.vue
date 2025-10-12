<template>
  <div class="steps">
    <div
      v-for="(step, index) in steps"
      :key="index"
      class="step"
    >
      <div class="step-number">{{ index + 1 }}</div>
      <div class="step-content">
        <h3>{{ step.title }}</h3>
        <p>{{ step.description }}</p>
        <img v-if="step.image" :src="step.image" :alt="step.title" />
      </div>
    </div>
  </div>
</template>

<script setup>
defineProps({
  steps: Array
})
</script>

<style scoped>
.steps {
  margin: 2rem 0;
}
.step {
  display: flex;
  gap: 1.5rem;
  margin-bottom: 2rem;
  padding: 1.5rem;
  background: var(--vp-c-bg-soft);
  border-radius: 8px;
}
.step-number {
  flex-shrink: 0;
  width: 40px;
  height: 40px;
  background: var(--vp-c-brand);
  color: white;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: bold;
  font-size: 20px;
}
.step-content img {
  margin-top: 1rem;
  max-width: 100%;
  border-radius: 4px;
}
</style>
