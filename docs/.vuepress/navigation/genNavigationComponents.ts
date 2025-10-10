import * as fs from 'fs'
import * as path from 'path'
import { default as matter } from 'gray-matter'
import { ThemeCollectionItem, ThemeNavItem, ThemeSidebarItem } from 'vuepress-theme-plume'

import { Locale } from './i18n'

interface MetaData {
  baseName: string
  order: number
  title: string
  icon: string
  index: boolean
}

interface NavigationComponents {
  navbar: ThemeNavItem[]
  collections: ThemeCollectionItem[]
}

type SidebarItem = ThemeSidebarItem | string

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

  // 文件名，不含扩展名
  const baseName = path.parse(entry.name).name
  // 获取顺序，目录的order在meta.dir.order里，文件的order在meta.order里，默认值为一个大数
  const order = Number((entry.isDirectory() ? meta?.dir?.order : meta?.order) ?? Number.MAX_SAFE_INTEGER)
  // 获取标题，先从matter里找title，再用正则获取一级标题，最后fallback到文件名（不含扩展名）
  const title = String(meta?.title ?? RegExp('# (.+)').exec(fileContent)?.[1] ?? baseName)
  // 获取图标
  const icon = String(meta?.icon ?? '')
  // 是否添加到索引，文件永远为true，目录则看meta.index，默认true
  const index = entry.isDirectory() ? (Boolean(meta?.index) ?? true) : true

  return {
    baseName: baseName,
    order: order,
    title: title,
    icon: icon,
    index: index,
  }
}

function getSidebarItems(dir: string): SidebarItem[] {
  interface WrappedSidebarItem {
    sidebarItem: SidebarItem
    order: number
  }

  const entries = fs.readdirSync(dir, { withFileTypes: true }).filter((e) => !e.name.startsWith('.'))

  const sidebarItemsWithOrder: WrappedSidebarItem[] = []
  for (const entry of entries) {
    let sidebarItem: SidebarItem

    const metaData = getMetaData(dir, entry)
    if (!metaData) {
      continue
    }

    if (entry.isDirectory()) {
      const children = getSidebarItems(path.join(dir, entry.name))
      // 可折叠的子目录
      sidebarItem = {
        text: metaData.title,
        // 只有当目录设置了index: true时，才生成链接，否则点击时不跳转、只切换折叠状态
        link: metaData.index ? `${metaData.baseName}/` : undefined,
        icon: metaData.icon,
        // 目前没有文档使用了badge这个特性，故不处理
        // badge: undefined,
        collapsed: true,
        // 前面不能加斜杠，必须用相对路径
        prefix: `${metaData.baseName}/`,
        items: children,
      }
    } else if (entry.isFile() && entry.name.endsWith('.md') && entry.name.toLowerCase() !== 'readme.md') {
      // 普通文件，取完整文件名作为链接
      sidebarItem = entry.name
    }

    sidebarItemsWithOrder.push({ sidebarItem: sidebarItem, order: metaData.order })
  }
  sidebarItemsWithOrder.sort((a, b) => a.order - b.order)
  return sidebarItemsWithOrder.map((i) => i.sidebarItem)
}

export function genNavigationComponents(
  locale: Locale,
  baseDir = path.resolve(__dirname, '../../'),
): NavigationComponents {
  interface WrappedNavigationComponent {
    navItem: ThemeNavItem
    collectionItem: ThemeCollectionItem
    order: number
  }

  const navigationComponentsWithOrder: WrappedNavigationComponent[] = []

  // 进入指定语言目录，即docs/<i18n>/
  const langDir = path.join(baseDir, locale.name)

  // 获取所有非隐藏文件和目录
  const entries = fs.readdirSync(langDir, { withFileTypes: true }).filter((e) => !e.name.startsWith('.'))

  for (const entry of entries) {
    if (!entry.isDirectory()) continue

    const metaData = getMetaData(langDir, entry)
    if (!metaData) {
      continue
    }

    const navbarItem: ThemeNavItem = {
      text: metaData.title,
      icon: metaData.icon,
      link: `/${locale.name}/${metaData.baseName}/`,
    }

    const collectionItem: ThemeCollectionItem = {
      type: 'doc',
      title: metaData.title,
      dir: metaData.baseName,
      linkPrefix: `/${metaData.baseName}/`,
      sidebar: getSidebarItems(path.join(langDir, entry.name)),
    }

    navigationComponentsWithOrder.push({
      navItem: navbarItem,
      collectionItem: collectionItem,
      order: metaData.order,
    })
  }

  navigationComponentsWithOrder.sort((a, b) => a.order - b.order)

  return {
    navbar: navigationComponentsWithOrder.map((i) => i.navItem),
    collections: navigationComponentsWithOrder.map((i) => i.collectionItem),
  }
}
