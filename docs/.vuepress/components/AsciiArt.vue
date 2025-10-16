<template>
  <div ref="asciiArtWrapperElement" class="ascii-art-wrapper">
    <pre ref="asciiArtContentElement" class="ascii-art-content">{{ asciiArtText }}</pre>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue'
import { getAsciiArt, ThemeType, AsciiArtScope } from '../plugins/asciiArt.mts'

// -------- Props --------
interface AsciiArtProps {
  name?: string
  theme?: ThemeType
  scope?: AsciiArtScope
}
const props = defineProps<AsciiArtProps>()

// -------- Refs --------
const asciiArtWrapperElement = ref<HTMLElement>()
const asciiArtContentElement = ref<HTMLElement>()

// -------- Data --------
let asciiArtNameInUse = props.name
const asciiArtText = ref('')

let layoutObserver: ResizeObserver
let themeObserver: MutationObserver

let isScaleUpLocked = false
let lastScaleRatio = 1

function refreshAsciiArt() {
  const asciiArtData = getAsciiArt(asciiArtNameInUse, props.theme, props.scope)
  asciiArtNameInUse = asciiArtData.name
  asciiArtText.value = asciiArtData.text
}

function scaleAsciiArt() {
  if (!asciiArtWrapperElement.value || !asciiArtContentElement.value) return

  // 原始高度和宽度（无视scale）
  const contentWidth = asciiArtContentElement.value.scrollWidth
  const contentHeight = asciiArtContentElement.value.scrollHeight
  if (contentWidth === 0 || contentHeight === 0) return

  const targetWidth = asciiArtWrapperElement.value.clientWidth
  const targetHeight = window.innerHeight

  const scaleRatio = Math.min(targetWidth / contentWidth, targetHeight / contentHeight)
  // 锁定状态不允许放大
  if (scaleRatio > lastScaleRatio && isScaleUpLocked) return

  lastScaleRatio = scaleRatio
  isScaleUpLocked = true

  asciiArtContentElement.value.style.transform = `scale(${scaleRatio})`
  asciiArtWrapperElement.value.style.height = `${contentHeight * scaleRatio}px`
}

function forceScaleAsciiArt() {
  isScaleUpLocked = false
  scaleAsciiArt()
}

onMounted(() => {
  if (!asciiArtWrapperElement.value || !asciiArtContentElement.value) return

  layoutObserver = new ResizeObserver(scaleAsciiArt)
  layoutObserver.observe(asciiArtContentElement.value)
  layoutObserver.observe(asciiArtWrapperElement.value)

  themeObserver = new MutationObserver(refreshAsciiArt)
  themeObserver.observe(document.documentElement, { attributes: true, attributeFilter: ['data-theme'] })

  window.addEventListener('resize', forceScaleAsciiArt)

  refreshAsciiArt()
})

onBeforeUnmount(() => {
  if (layoutObserver) {
    layoutObserver.disconnect()
  }
  if (themeObserver) {
    themeObserver.disconnect()
  }
  window.removeEventListener('resize', forceScaleAsciiArt)
})
</script>

<style scoped>
.ascii-art-wrapper {
  display: flex;
  justify-content: center;
  align-items: flex-start;
}
.ascii-art-content {
  display: block;
  white-space: pre;
  margin: 0 auto;
  transform-origin: top;
  line-height: 1;
  font-family: 'JetBrains Mono', monospace;
}
</style>
