import { defineClientConfig } from 'vuepress/client'

import { getAsciiArt } from './plugins/asciiArt.mts'

import AsciiArt from './components/AsciiArt.vue'
import ImageGrid from './components/ImageGrid.vue'
import Redirect from './components/Redirect.vue'

import './styles/index.scss'

export default defineClientConfig({
  enhance: ({ app }) => {
    app.component('AsciiArt', AsciiArt)
    app.component('ImageGrid', ImageGrid)
    app.component('Redirect', Redirect)

    // 输出一个随机的字符画
    const art = getAsciiArt(undefined, 'auto', 'console')
    console.log('%c' + art, 'white-space: pre;')
  },
})
