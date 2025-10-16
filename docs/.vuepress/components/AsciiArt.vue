<template>
  <div ref="asciiArtWrapper" class="ascii-art-wrapper">
    <pre ref="asciiArt" class="ascii-art">{{ art }}</pre>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount, computed } from 'vue'
import { getAsciiArt, ThemeType } from '../plugins/asciiArt.mts'

// -------- Props --------
interface AsciiArtProps {
  name?: string
  theme?: ThemeType
}
const props = defineProps<AsciiArtProps>()

// -------- Refs --------
const asciiArtWrapper = ref<HTMLElement>()
const asciiArt = ref<HTMLElement>()

// -------- Data --------
const art = computed(() => getAsciiArt(props.name, props.theme))

let observer: ResizeObserver | null = null

let isScaleUpLocked = false
let lastScaleRatio = 1

function scaleAsciiArt() {
  if (!asciiArtWrapper.value || !asciiArt.value) return

  // 原始高度和宽度（无视scale）
  const contentWidth = asciiArt.value.scrollWidth
  const contentHeight = asciiArt.value.scrollHeight
  if (contentWidth === 0 || contentHeight === 0) return

  const targetWidth = asciiArtWrapper.value.clientWidth
  const targetHeight = window.innerHeight

  const scaleRatio = Math.min(
    targetWidth / contentWidth,
    targetHeight / contentHeight,
  )
  // 锁定状态不允许放大
  if (scaleRatio > lastScaleRatio && isScaleUpLocked) return

  lastScaleRatio = scaleRatio
  isScaleUpLocked = true

  asciiArt.value.style.transform = `scale(${scaleRatio})`
  asciiArtWrapper.value.style.height = `${contentHeight * scaleRatio}px`
}

onMounted(() => {
  if (!asciiArtWrapper.value || !asciiArt.value) return

  observer = new ResizeObserver(scaleAsciiArt)
  observer.observe(asciiArt.value)
  observer.observe(asciiArtWrapper.value)

  window.addEventListener('resize', () => {
    isScaleUpLocked = false
    scaleAsciiArt()
  })
})

onBeforeUnmount(() => {
  if (observer) {
    observer.disconnect()
  }
})
</script>

<style scoped>
.ascii-art-wrapper {
  display: flex;
  justify-content: center;
  align-items: flex-start;
}
.ascii-art {
  display: block;
  white-space: pre;
  margin: 0 auto;
  transform-origin: top;
  line-height: 1;
  font-family: 'JetBrains Mono', monospace;
}
</style>
