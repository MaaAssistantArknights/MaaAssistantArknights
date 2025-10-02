import { defineClientConfig } from 'vuepress/client'

import { getAsciiArt } from './asciiArtService.mts'

import AsciiArt from './components/AsciiArt.vue'
import ImageGrid from './components/ImageGrid.vue'

import './styles/index.scss'

export default defineClientConfig({
  enhance: ({ app }) => {
    app.component('AsciiArt', AsciiArt)
    app.component('ImageGrid', ImageGrid)

    // 输出一个特定的字符画
    const art = getAsciiArt('maa-logo-with-endorsement', 'disable')
    console.log('%c' + art, 'white-space: pre;')
  },
})
