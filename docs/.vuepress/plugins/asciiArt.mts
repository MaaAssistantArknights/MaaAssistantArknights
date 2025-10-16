const modules = import.meta.glob<string>('../../asciiArts/*.txt', { query: '?raw', import: 'default', eager: true })

const asciiArts: Record<string, string> = {}
for (const path in modules) {
  const name = path
    .split('/')
    .pop()
    ?.replace(/\.txt$/, '')!
  asciiArts[name] = modules[path]
}

export type ThemeType = 'auto' | 'disable' | string
export type AsciiArtScope = 'web' | 'console'
interface AsciiArtData {
  name: string | null
  text: string
}

function pickAsciiArt(
  arts: Record<string, string>,
  artFilter?: (key: string) => boolean,
  keyToName?: (key: string) => string,
): AsciiArtData {
  const keys = artFilter ? Object.keys(arts).filter(artFilter) : Object.keys(arts)

  let asciiArtKey: string | null
  let asciiArtText: string

  if (keys.length === 0) {
    asciiArtKey = null
    asciiArtText = ''
  } else if (keys.length === 1) {
    asciiArtKey = keys[0]
    asciiArtText = arts[keys[0]]
  } else {
    const randomKey = keys[Math.floor(Math.random() * keys.length)]
    asciiArtKey = randomKey
    asciiArtText = arts[randomKey]
  }
  if (!asciiArtKey || !keyToName) {
    return { name: asciiArtKey, text: asciiArtText }
  } else {
    return { name: keyToName(asciiArtKey), text: asciiArtText }
  }
}

export function getAsciiArt(name?: string, theme: ThemeType = 'auto', scope: AsciiArtScope = 'web'): AsciiArtData {
  let resolvedTheme: ThemeType = theme

  // auto 模式
  if (theme === 'auto') {
    let currentTheme: string | null = null
    if (scope === 'web' && typeof document !== 'undefined') {
      // 浏览器中读取 HTML data-theme
      currentTheme = document?.documentElement?.getAttribute('data-theme')
    } else if (scope === 'console' && typeof window !== 'undefined' && window.matchMedia) {
      // fallback: 检查系统首选主题
      currentTheme = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
    }
    resolvedTheme = currentTheme ? currentTheme : 'disable'
  }

  if (resolvedTheme !== 'disable') {
    // light/dark/... 模式
    return name
      ? pickAsciiArt(
          asciiArts,
          (k) => k === name + '.' + resolvedTheme,
          () => name,
        )
      : pickAsciiArt(
          asciiArts,
          (k) => k.endsWith('.' + resolvedTheme),
          (k) => k.replace(new RegExp('\\.' + resolvedTheme + '$'), ''),
        )
  } else {
    // disable 模式
    return name
      ? pickAsciiArt(
          asciiArts,
          (k) => k === name || k.startsWith(name + '.'),
          () => name,
        )
      : pickAsciiArt(
          asciiArts,
          () => true,
          (k) => k,
        )
  }
}
