const modules = import.meta.glob<string>('../asciiArts/*.txt', { query: '?raw', import: 'default', eager: true })

const asciiArts: Record<string, string> = {}
for (const path in modules) {
  const name = path
    .split('/')
    .pop()
    ?.replace(/\.txt$/, '')!
  asciiArts[name] = modules[path]
}

export type ThemeType = 'auto' | 'disable' | string

function pickAsciiArt(arts: Record<string, string>, artFilter?: (key: string) => boolean): string {
  const keys = artFilter ? Object.keys(arts).filter(artFilter) : Object.keys(arts)

  if (keys.length === 0) return ''
  if (keys.length === 1) return arts[keys[0]]

  const randomKey = keys[Math.floor(Math.random() * keys.length)]
  return arts[randomKey]
}

export function getAsciiArt(name?: string, theme: ThemeType = 'auto'): string {
  let resolvedTheme: ThemeType = theme

  // auto 模式
  if (theme === 'auto') {
    let current: string | null = null
    if (typeof document !== 'undefined') {
      // 浏览器中读取 HTML data-theme
      current = document?.documentElement?.getAttribute('data-theme')
    } else if (typeof window !== 'undefined' && window.matchMedia) {
      // fallback: 检查系统首选主题
      current = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
    }
    resolvedTheme = current ? current : 'disable'
  }

  if (resolvedTheme !== 'disable') {
    // light/dark/... 模式
    return name
      ? pickAsciiArt(asciiArts, (k) => k === name + '.' + resolvedTheme)
      : pickAsciiArt(asciiArts, (k) => k.endsWith('.' + resolvedTheme))
  } else {
    // disable 模式
    return name
      ? pickAsciiArt(asciiArts, (k) => k === name || k.startsWith(name + '.'))
      : pickAsciiArt(asciiArts, () => true)
  }
}
