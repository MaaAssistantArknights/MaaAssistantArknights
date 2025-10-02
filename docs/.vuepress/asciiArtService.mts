const modules = import.meta.glob<string>('../asciiArts/*.txt', { query: '?raw', import: 'default', eager: true })

const asciiArts: Record<string, string> = {}
for (const path in modules) {
  const name = path
    .split('/')
    .pop()
    ?.replace(/\.txt$/, '')!
  asciiArts[name] = modules[path]
}

export type ThemeType = 'light' | 'dark' | 'auto' | 'disable'

export function getAsciiArt(name?: string, theme: ThemeType = 'auto'): string {
  let targetTheme = ''
  if (theme === 'disable') {
    targetTheme = ''
  } else if (theme === 'auto') {
    targetTheme = '.' + document.documentElement.getAttribute('data-theme')
  } else {
    targetTheme = '.' + theme
  }

  // 如果指定了 name
  if (name && asciiArts[name + targetTheme]) {
    return asciiArts[name + targetTheme]
  }

  // 随机选取
  const keys = Object.keys(asciiArts)

  // 根据主题过滤
  let filteredKeys: string[]
  if (theme === 'disable') {
    // 不做后缀过滤，直接用全部 keys
    filteredKeys = keys
  } else {
    // 其它情况，只留下匹配到对应后缀的
    filteredKeys = keys.filter((k) => k.endsWith(targetTheme))
  }

  // 如果过滤后没找到，就退回全部 keys
  if (filteredKeys.length === 0) filteredKeys = keys

  const randomKey = filteredKeys[Math.floor(Math.random() * filteredKeys.length)]
  return asciiArts[randomKey]
}
