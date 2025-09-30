<template>
  <div ref="wrapper" class="ascii-wrapper">
    <pre ref="pre" class="ascii-art">{{ art }}</pre>
  </div>
</template>

<script lang="ts">
import { defineComponent, computed, onMounted, ref, watch, nextTick, onBeforeUnmount } from 'vue'
import { getAsciiArt } from '../asciiArtService'

export default defineComponent({
  name: 'AsciiArt',
  props: {
    name: { type: String, required: false },
  },
  setup(props) {
    const wrapper = ref<HTMLElement>()
    const pre = ref<HTMLElement>()
    const art = computed(() => getAsciiArt(props.name))

    const baseFontSize = ref(0)
    const baseLineHeight = ref(0)
    const basePreWidth = ref(0)

    const updateBaseMetrics = () => {
      if (!pre.value) return
      const style = window.getComputedStyle(pre.value)
      baseFontSize.value = parseFloat(style.fontSize)
      baseLineHeight.value = parseFloat(style.lineHeight)
      basePreWidth.value = pre.value.scrollWidth // 原始字符画宽度
    }

    const adjustFont = () => {
      if (!wrapper.value || !pre.value || !basePreWidth.value) return

      const wrapperWidth = wrapper.value.clientWidth
      let scale = wrapperWidth / basePreWidth.value
      if (scale > 1) scale = 1 // 不放大超过原始宽度

      // 缩放字体大小
      let newFontSize = baseFontSize.value * scale
      pre.value.style.fontSize = newFontSize + 'px'

      // 按比例调整行高
      const ratio = baseLineHeight.value / baseFontSize.value
      pre.value.style.lineHeight = newFontSize * ratio + 'px'
    }

    onMounted(() => {
      nextTick(() => {
        updateBaseMetrics()
        adjustFont()
      })
      window.addEventListener('resize', adjustFont)
    })

    onBeforeUnmount(() => {
      window.removeEventListener('resize', adjustFont)
    })

    // 当 art 变化时重新记录宽度和基准
    watch(art, async () => {
      await nextTick()
      updateBaseMetrics()
      adjustFont()
    })

    return { wrapper, pre, art }
  },
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
}
</style>
