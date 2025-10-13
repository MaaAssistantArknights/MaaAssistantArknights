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

  const scaleRatio = asciiArtWrapper.value.clientWidth / asciiArt.value.scrollWidth
  // 锁定状态不允许放大
  if (scaleRatio > lastScaleRatio && isScaleUpLocked) return

  lastScaleRatio = scaleRatio
  isScaleUpLocked = true

  asciiArt.value.style.transform = `scale(${scaleRatio})`
  asciiArtWrapper.value.style.height = `${asciiArt.value.scrollHeight * scaleRatio}px`
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
