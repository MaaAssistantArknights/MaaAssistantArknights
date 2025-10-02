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

function scaleFont(
  currentFontSize: number,
  currentWidth: number,
  targetWidth: number,
): { newFontSize: number; newLineHeight: number } {
  // 注意：不要使用小数字号！
  // iOS系统浏览器会用“这个小数字号”来渲染宽度，
  // 但是会用“这个小数字号无条件舍去所得的整数”来渲染高度，
  // 导致字符画变扁
  const ratio = targetWidth / currentWidth

  // 无条件舍去
  const newFontSize = Math.floor(currentFontSize * ratio)
  return {
    newFontSize: newFontSize,
    newLineHeight: newFontSize, // 字符画的行高与字体大小保持一致
  }
}

function waitForStableScrollWidth(
  el: HTMLElement,
  stableThreshold: number, // 若连续 stableThreshold 次测量所得宽度值均相同，则认为已经稳定
  timeoutThreshold: number, // 若在 timeoutThreshold 次测量后仍未稳定，则记为超时
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

  // 等待等宽字体加载完毕
  // 若连续 11 次测量所得宽度值均相同，则认为已经稳定
  // 若在 451 次测量后仍未稳定，则记为超时
  // 这两个数字是“随机”拟定的
  const isStable = await waitForStableScrollWidth(pre.value, 11, 451)
  if (!isStable) {
    // 超时则不调整字符画的字体大小
    pre.value.style.overflowX = 'auto'
    return
  }

  // 监听 wrapper 的宽度或高度变化
  observer = new ResizeObserver(() => {
    if (!wrapper.value || !pre.value) return

    const currentFontSize = parseFloat(getComputedStyle(pre.value).fontSize)
    const currentWidth = pre.value.scrollWidth
    const targetWidth = wrapper.value.clientWidth

    // 计算新的值
    const { newFontSize, newLineHeight } = scaleFont(currentFontSize, currentWidth, targetWidth)

    // 应用到字符画元素
    pre.value.style.fontSize = newFontSize + 'px'
    pre.value.style.lineHeight = newLineHeight + 'px'
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
  line-height: 1;
  max-width: 100%;
  overflow: hidden;
  font-family: 'JetBrains Mono', monospace;
}
</style>
