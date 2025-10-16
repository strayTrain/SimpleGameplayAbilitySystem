<template>
  <div class="markdown-actions">
    <button @click="copyMarkdown" class="action-button">
      <span class="icon">📋</span>
      <span>{{ copyButtonText }}</span>
    </button>
    <button @click="downloadMarkdown" class="action-button">
      <span class="icon">⬇️</span>
      <span>Download as File</span>
    </button>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const props = defineProps({
  content: {
    type: String,
    required: true
  },
  filename: {
    type: String,
    default: 'document.md'
  }
})

const copyButtonText = ref('Copy Markdown')

const copyMarkdown = async () => {
  try {
    await navigator.clipboard.writeText(props.content)
    copyButtonText.value = '✓ Copied!'
    setTimeout(() => {
      copyButtonText.value = 'Copy Markdown'
    }, 2000)
  } catch (err) {
    console.error('Failed to copy:', err)
    copyButtonText.value = 'Failed to copy'
    setTimeout(() => {
      copyButtonText.value = 'Copy Markdown'
    }, 2000)
  }
}

const downloadMarkdown = () => {
  const blob = new Blob([props.content], { type: 'text/markdown' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = props.filename
  document.body.appendChild(link)
  link.click()
  document.body.removeChild(link)
  URL.revokeObjectURL(url)
}
</script>

<style scoped>
.markdown-actions {
  display: flex;
  gap: 12px;
  margin-bottom: 24px;
  padding: 16px;
  background: var(--vp-c-bg-soft);
  border-radius: 8px;
  border: 1px solid var(--vp-c-divider);
}

.action-button {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 16px;
  background: var(--vp-button-brand-bg);
  color: var(--vp-button-brand-text);
  border: none;
  border-radius: 6px;
  font-size: 14px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s ease;
}

.action-button:hover {
  background: var(--vp-button-brand-hover-bg);
  transform: translateY(-1px);
}

.action-button:active {
  transform: translateY(0);
}

.action-button .icon {
  font-size: 16px;
}

@media (max-width: 640px) {
  .markdown-actions {
    flex-direction: column;
  }

  .action-button {
    width: 100%;
    justify-content: center;
  }
}
</style>
