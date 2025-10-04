import * as fs from 'fs'
import * as path from 'path'
import * as matterModule from 'gray-matter'
import { defineNoteConfig, ThemeNote, ThemeSidebarItem } from 'vuepress-theme-plume'

const matter = (matterModule as any).default

interface MetaData {
  baseName: string
  order: number
  title: string
  icon: string
  index: boolean
}

function getMetaData(dir: string, entry: fs.Dirent): MetaData | null {
  const currentPath = path.join(dir, entry.name)
  if (!fs.existsSync(currentPath)) {
    return null
  }

  let mdFilePath = ''
  if (entry.isDirectory()) {
    mdFilePath = path.join(currentPath, 'README.md')
  } else if (entry.isFile() && entry.name.endsWith('.md') && entry.name.toLowerCase() !== 'readme.md') {
    mdFilePath = currentPath
  } else {
    return null
  }

  if (!fs.existsSync(mdFilePath)) {
    return null
  }

  const fileContent = fs.readFileSync(mdFilePath, 'utf-8')
  const meta = matter(fileContent).data ?? {}

  const baseName = path.parse(entry.name).name
  // 获取顺序，目录的order在meta.dir.order里，文件的order在meta.order里，默认值为一个大数
  const order = Number((entry.isDirectory() ? meta?.dir?.order : meta?.order) ?? Number.MAX_SAFE_INTEGER)
  // 获取标题，先从matter里找title，再用正则获取一级标题，最后fallback到文件名（不含扩展名）
  const title = String(meta?.title ?? RegExp('# (.+)').exec(fileContent)?.[1] ?? baseName)
  // 获取图标
  const icon = String(meta?.icon ?? '')
  // 是否作为索引页，文件永远为true，目录则看meta.index，默认true
  const index = entry.isDirectory() ? (Boolean(meta?.index) ?? true) : true

  return {
    baseName: baseName,
    order: order,
    title: title,
    icon: icon,
    index: index,
  }
}

export function genNotes(lang: string, baseDir = path.resolve(__dirname, '../../')): ThemeNote[] {
  // 进入指定语言目录，即docs/<i18n>/
  const langDir = path.join(baseDir, lang)

  // 递归获取目录和文件
  function getItems(dir: string, isRoot: boolean): any[] {
    // 将内容与对应顺序进行包装
    interface Wrapped {
      content: ThemeNote | ThemeSidebarItem | string
      order: number
    }
    let itemsWithOrder: Wrapped[] = []

    // 获取所有非隐藏文件和目录
    const entries = fs.readdirSync(dir, { withFileTypes: true }).filter((e) => !e.name.startsWith('.'))

    for (const entry of entries) {
      const metaData = getMetaData(dir, entry)
      if (!metaData) {
        continue
      }
      if (entry.isDirectory()) {
        // 递归获取子目录内容
        const children = getItems(path.join(dir, entry.name), false)

        if (isRoot) {
          // 一级目录，作为“专题”
          const item = defineNoteConfig({
            dir: metaData.baseName,
            link: `/${metaData.baseName}/`,
            text: metaData.title,
            sidebar: children,
          })
          itemsWithOrder.push({ content: item, order: metaData.order })
        } else {
          // 非一级目录，作为可折叠的子目录
          const item: ThemeSidebarItem = {
            text: metaData.title,
            // 只有当目录设置了index: true时，才生成链接，否则点击时不跳转、只切换折叠状态
            link: metaData.index ? `${metaData.baseName}/` : undefined,
            icon: metaData.icon,
            // 目前没有文档使用了这个特性，故不处理
            // badge: undefined,
            collapsed: true,
            // 前面不能加斜杠，必须用相对路径
            prefix: `${metaData.baseName}/`,
            items: children,
          }
          itemsWithOrder.push({ content: item, order: metaData.order })
        }
      } else if (entry.isFile() && entry.name.endsWith('.md') && entry.name.toLowerCase() !== 'readme.md') {
        // 普通文件，取完整文件名作为链接
        const item = entry.name
        itemsWithOrder.push({ content: item, order: metaData.order })
      }
    }
    // 当前dir的内容读取完毕，进行排序并返回，返回时丢弃order
    itemsWithOrder.sort((a, b) => a.order - b.order)
    return itemsWithOrder.map((i) => i.content)
  }
  // 递归起点，只有这里是一级目录，isRoot传true
  return getItems(langDir, true)
}
