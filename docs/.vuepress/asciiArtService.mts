const modules = import.meta.glob<string>('../asciiArts/*.txt', { query: '?raw', import: 'default', eager: true })

const asciiArts: Record<string, string> = {}
for (const path in modules) {
  const name = path
    .split('/')
    .pop()
    ?.replace(/\.txt$/, '')!
  asciiArts[name] = modules[path]
}

export function getAsciiArt(name?: string): string {
  if (name && asciiArts[name]) return asciiArts[name]
  const keys = Object.keys(asciiArts)
  return asciiArts[keys[Math.floor(Math.random() * keys.length)]]
}
