import * as fs from 'fs';
import * as path from 'path';
import { default as matter } from 'gray-matter';
import { defineNoteConfig, ThemeNavItem, ThemeNote, ThemeSidebarItem } from 'vuepress-theme-plume';

import { Locale } from './i18n';

interface MetaData {
  baseName: string
  order: number
  title: string
  icon: string
  index: boolean
}

interface NavigationComponents {
  navbar: ThemeNavItem[];
  notes: ThemeNote[];
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

  // 文件名，不含扩展名
  const baseName = path.parse(entry.name).name;
  // 获取顺序，目录的order在meta.dir.order里，文件的order在meta.order里，默认值为一个大数
  const order = Number((entry.isDirectory() ? meta?.dir?.order : meta?.order) ?? Number.MAX_SAFE_INTEGER)
  // 获取标题，先从matter里找title，再用正则获取一级标题，最后fallback到文件名（不含扩展名）
  const title = String(meta?.title ?? RegExp('# (.+)').exec(fileContent)?.[1] ?? baseName)
  // 获取图标
  const icon = String(meta?.icon ?? '');
  // 是否添加到索引，文件永远为true，目录则看meta.index，默认true
  const index = entry.isDirectory() ? (Boolean(meta?.index) ?? true) : true;

  return {
    baseName: baseName,
    order: order,
    title: title,
    icon: icon,
    index: index,
  }
}

export function genNavigationComponents(
  locale: Locale,
  baseDir = path.resolve(__dirname, '../../'),
): NavigationComponents {
  // 将内容与对应顺序进行包装
  type NavbarItem = ThemeNavItem;
  type SidebarItem = ThemeNote | ThemeSidebarItem | string;

  interface IntermediateNavigationComponents {
    navbar: NavbarItem[];
    notes: SidebarItem[];
  }

  interface WrappedNavbarItem {
    content: NavbarItem;
    order: number;
  }

  interface WrappedSidebarItem {
    content: SidebarItem;
    order: number;
  }

  // 进入指定语言目录，即docs/<i18n>/
  const langDir = path.join(baseDir, locale.name);

  // 递归获取目录和文件
  function getItems(dir: string, isRoot: boolean): IntermediateNavigationComponents {
    const navbarItemsWithOrder: WrappedNavbarItem[] = [];
    const sidebarItemsWithOrder: WrappedSidebarItem[] = [];

    // 获取所有非隐藏文件和目录
    const entries = fs.readdirSync(dir, { withFileTypes: true }).filter((e) => !e.name.startsWith('.'))

    for (const entry of entries) {
      const metaData = getMetaData(dir, entry)
      if (!metaData) {
        continue
      }
      let sidebarItem: SidebarItem;
      let navbarItem: NavbarItem;

      if (entry.isDirectory()) {
        // 递归获取子目录内容
        const children = getItems(path.join(dir, entry.name), false)

        if (isRoot) {
          // 一级目录，作为“专题”
          navbarItem = {
            text: metaData.title,
            icon: metaData.icon,
            link: `/${locale.name}/${metaData.baseName}/`,
          };
          // 只在当前条件下，才会有navbarItem
          navbarItemsWithOrder.push({ content: navbarItem, order: metaData.order });

          sidebarItem = defineNoteConfig({
            dir: metaData.baseName,
            link: `/${metaData.baseName}/`,
            text: metaData.title,
            sidebar: children.notes,
          });
        } else {
          // 非一级目录，作为可折叠的子目录
          sidebarItem = {
            text: metaData.title,
            // 只有当目录设置了index: true时，才生成链接，否则点击时不跳转、只切换折叠状态
            link: metaData.index ? `${metaData.baseName}/` : null,
            icon: metaData.icon,
            // 目前没有文档使用了这个特性，故不处理
            // badge: undefined,
            collapsed: true,
            // 前面不能加斜杠，必须用相对路径
            prefix: `${metaData.baseName}/`,
            items: children.notes,
          };
        }
      } else if (entry.isFile() && entry.name.endsWith('.md') && entry.name.toLowerCase() !== 'readme.md') {
        // 普通文件，取完整文件名作为链接
        sidebarItem = entry.name;
      }
      sidebarItemsWithOrder.push({ content: sidebarItem, order: metaData.order });
    }
    // 当前dir的内容读取完毕，进行排序并返回，返回时丢弃order
    if (isRoot) {
      navbarItemsWithOrder.sort((a, b) => a.order - b.order);
    }
    sidebarItemsWithOrder.sort((a, b) => a.order - b.order);
    return {
      navbar: navbarItemsWithOrder.map((i) => i.content),
      notes: sidebarItemsWithOrder.map((i) => i.content),
    };
  }
  // 递归起点，只有这里是一级目录，isRoot传true
  return getItems(langDir, true) as NavigationComponents;
}
