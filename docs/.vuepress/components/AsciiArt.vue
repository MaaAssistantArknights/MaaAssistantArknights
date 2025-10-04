<template>
  <div ref="wrapper" class="ascii-wrapper">
    <pre ref="pre" class="ascii-art">{{ art }}</pre>
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
const wrapper = ref<HTMLElement>()
const pre = ref<HTMLElement>()

// -------- Data --------
const art = computed(() => getAsciiArt(props.name, props.theme))

let observer: ResizeObserver | null = null

// 锁定状态下不能增加 fontSize
let isScaleLocked = false

function scaleFont(
  currentFontSize: number,
  currentWidth: number,
  targetWidth: number,
): { newFontSize: number; newLineHeight: number } {
  let newFontSize: number

  if (isScaleLocked && currentWidth <= targetWidth) {
    // 锁定状态不自动放大字号
    newFontSize = currentFontSize
  } else {
    // 此次调整依旧
    if (currentWidth > targetWidth) {
      // 设置锁定状态防止立刻回弹
      isScaleLocked = true
    }
    // 注意：font-size 不要使用小数！
    // iOS系统浏览器会用“这个小数”来渲染宽度，
    // 但是会用“这个小数无条件舍去所得的整数”来渲染高度，
    // 导致字符画变扁。
    const ratio = targetWidth / currentWidth

    // 无条件舍去
    newFontSize = ratio !== Infinity ? Math.floor(currentFontSize * ratio) : currentFontSize
  }

  return {
    newFontSize: newFontSize,
    newLineHeight: newFontSize, // 字符画的行高与字体大小保持一致
  }
}

function adjustLayout() {
  if (!wrapper.value || !pre.value) return

  const currentFontSize = parseFloat(getComputedStyle(pre.value).fontSize)
  const currentWidth = pre.value.scrollWidth
  const targetWidth = wrapper.value.clientWidth

  // 计算新的值
  const { newFontSize, newLineHeight } = scaleFont(currentFontSize, currentWidth, targetWidth)

  // 应用到字符画元素
  pre.value.style.fontSize = newFontSize + 'px'
  pre.value.style.lineHeight = newLineHeight + 'px'
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
        // 宽度每变一次就调整一次
        adjustLayout()
        lastWidth = currentWidth
        stableCount = 0
      }

      if (stableCount >= stableThreshold) {
        resolve(true)
        // 只resolve不return，继续监测直到超时
        // 因为首次加载时，字体完全渲染可能需要较长时间
        // 先继续初始化进程，同时持续监测，宽度变化后立即触发一次调整
        // return
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
  observer = new ResizeObserver(adjustLayout)

  observer.observe(wrapper.value)

  window.addEventListener('resize', () => {
    isScaleLocked = false
    adjustLayout() // 解锁后立即尝试重新调整字体
  })
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
