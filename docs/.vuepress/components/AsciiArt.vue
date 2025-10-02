<template>
  <div ref="wrapper" class="ascii-wrapper">
    <pre ref="pre" class="ascii-art">{{ art }}</pre>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount, computed } from 'vue'
import { getAsciiArt } from '../asciiArtService.mts'

// -------- Props --------
const props = defineProps<{ name?: string }>()

// -------- Refs --------
const wrapper = ref<HTMLElement>()
const pre = ref<HTMLElement>()

// -------- Data --------
const art = computed(() => getAsciiArt(props.name))

let observer: ResizeObserver | null = null
let observerTmp: ResizeObserver | null = null

function scaleFont(
  currentFontSize: number,
  currentLineHeight: number,
  currentWidth: number,
  targetWidth: number,
): { fontSize: number; lineHeight: number } {
  const ratio = targetWidth / currentWidth
  return {
    fontSize: currentFontSize * ratio,
    lineHeight: currentLineHeight * ratio,
  }
}

function waitForStableScrollWidth(
  el: HTMLElement,
  stableThreshold: number,
  timeoutThreshold: number,
): Promise<boolean> {
  return new Promise<boolean>((resolve) => {
    let totalCount = 0
    let stableCount = 0
    let lastWidth = -1

    const stableCheck = () => {
      totalCount++
      const currentWidth = el.scrollWidth
      if (currentWidth === lastWidth) {
        stableCount++
      } else {
        lastWidth = currentWidth
        stableCount = 0
      }
      if (stableCount >= stableThreshold) {
        resolve(true)
        return
      }
      if (totalCount >= timeoutThreshold) {
        resolve(false)
        return
      }
      requestAnimationFrame(stableCheck)
    }
    requestAnimationFrame(stableCheck)
  })
}

onMounted(async () => {
  if (!wrapper.value || !pre.value) return

  const isStable = await waitForStableScrollWidth(pre.value, 11, 451)
  if (!isStable) {
    pre.value.style.overflowX = 'auto'
    return
  }

  // 监听 wrapper 宽度变化
  observer = new ResizeObserver(() => {
    if (!wrapper.value || !pre.value) return

    // 初始字号和行高
    const baseFontSize = parseFloat(getComputedStyle(pre.value).fontSize)
    const baseLineHeight = parseFloat(getComputedStyle(pre.value).lineHeight)
    const baseWidth = pre.value.scrollWidth

    const targetWidth = wrapper.value.clientWidth

    // 用纯函数计算新的值
    const { fontSize, lineHeight } = scaleFont(baseFontSize, baseLineHeight, baseWidth, targetWidth)

    // 应用到 pre
    pre.value.style.fontSize = fontSize + 'px'
    pre.value.style.lineHeight = lineHeight + 'px'
  })

  observer.observe(wrapper.value)
})

onBeforeUnmount(() => {
  if (observer && wrapper.value) {
    observer.unobserve(wrapper.value)
    observer.disconnect()
  }
})
</script>

<style scoped>
.ascii-wrapper {
  display: flex;
  justify-content: center;
}
.ascii-art {
  display: block;
  white-space: pre;
  margin: 0 auto;
  line-height: 1.3;
  max-width: 100%;
  overflow: hidden;
}
</style>
