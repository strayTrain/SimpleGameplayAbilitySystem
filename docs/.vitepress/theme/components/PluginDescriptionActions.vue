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
import { ref, onMounted } from 'vue'

const copyButtonText = ref('Copy Markdown')
const markdownContent = ref('')

// Clean markdown content by removing Vue component and tip callout
const cleanMarkdownContent = (content) => {
  // Remove the PluginDescriptionActions component line and any surrounding newlines
  content = content.replace(/<PluginDescriptionActions \/>\s*/g, '')

  // Remove the tip callout block (:::tip ... :::) with all content between
  // Use a more robust pattern that handles various newline characters
  content = content.replace(/:::tip[^\n]*\n.*?:::/gs, '')

  // Clean up any excessive newlines left behind (more than 2 consecutive newlines)
  content = content.replace(/\n{3,}/g, '\n\n')

  return content.trim()
}

// Fetch the raw markdown content
onMounted(async () => {
  try {
    const response = await fetch('/SimpleGameplayAbilitySystem/plugin_description.md?raw=true')
    if (response.ok) {
      const rawContent = await response.text()
      markdownContent.value = cleanMarkdownContent(rawContent)
    }
  } catch (err) {
    console.error('Failed to fetch markdown:', err)
  }
})

const copyMarkdown = async () => {
  if (!markdownContent.value) {
    copyButtonText.value = 'Content not loaded'
    setTimeout(() => {
      copyButtonText.value = 'Copy Markdown'
    }, 2000)
    return
  }

  try {
    await navigator.clipboard.writeText(markdownContent.value)
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
  if (!markdownContent.value) {
    return
  }

  const blob = new Blob([markdownContent.value], { type: 'text/markdown' })
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = 'plugin_description.md'
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
  margin: 24px 0;
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
