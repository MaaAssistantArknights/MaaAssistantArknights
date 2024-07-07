---
order: 6
icon: jam:write-f
---

# 文件編寫指南

::: tip
本文件的目的在於指導文件編寫者更好地使用主題提供的功能，以達到更易讀的效果。
:::

我們的文件基於 [vuepress](https://github.com/vuejs/vuepress) 構建，使用了 [vuepress-theme-hope](https://github.com/vuepress-theme-hope/vuepress-theme-hope) 主題，你也可以查看[官方文件](https://theme-hope.vuejs.press/zh/)來獲取更詳細的說明，這裡僅介紹一些常用的功能。

## 本地部署

1. 安裝 [pnpm](https://pnpm.io/zh/installation)，並參考 [Pull Request 流程簡述](./development.md#github-pull-request-流程簡述)將倉庫克隆到本地。
2. 在 `website` 目錄下新建終端，運行 `pnpm i` 下載依賴。
3. 運行 `pnpm run dev` 進行部署。

## 容器

~~不是 docker 那個容器~~

該主題提供了關於提示、注釋、信息、注意、警告和詳情自定義容器的支持，我們可以利用這一特性來強調部分內容。

容器的使用方法：

```markdown
::: [容器類型] [容器標題（可選）]
你想寫的內容
:::
```

接受的容器内容与其默认标题如下：

- `tip` 提示
- `note` 注釋
- `info` 信息
- `warning` 注意
- `danger` 警告
- `details` 詳情

### 容器示例

::: tip
這是提示容器
:::

::: note
這是注釋容器
:::

::: info
這是信息容器
:::

::: warning
這是注意容器
:::

::: danger
這是危險容器
:::

::: details
這是詳情容器
:::

## 圖標

該主題提供了圖標支持，你可以在以下地方使用圖標：

- 文件標題：在 frontmatter 中設置文件標題旁邊的圖標

- 導航欄/側邊欄：設置在導航欄與側邊欄中顯示的圖標

- 文件內容：在文件中使用圖標

### 設置文件的圖標

你可以在文件的 [frontmatter](#frontmatter) 中使用 icon 來設置文件的圖標。

这个图标会显示在文档标题的旁边。

::: details 本文件的 frontmatter 設置

```markdown
---
icon: jam:write-f
---
```

:::

### 在文件中使用圖標

你可以使用 `<HopeIcon />` 組件在 markdown 中添加圖標。該組件有以下屬性：

- `icon` 接受圖標關鍵字及 Url，如 `jam:write-f`，`ic:round-home` 等
- `color` 接受 css 風格的顏色值，如 `#fff`，`red` 等（該選項僅對 svg 圖標有效）
- `size` 接受 css 風格的大小，如 `1rem`，`2em`，`100px` 等

::: details 案例
<HopeIcon icon="ic:round-home" color="#1f1e33"/>

```markdown
<HopeIcon icon="ic:round-home" color="#1f1e33"/>
```

<HopeIcon icon="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" size="4rem" />
```markdown
<HopeIcon icon="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" size="4rem" />
```
:::

### 圖標關鍵字的獲取

本文件使用的圖標來自於 [iconify](https://iconify.design/)，你可以在其給出的 [圖標搜索界面](https://icon-sets.iconify.design/) 中搜索你想要的圖標，然後復制其關鍵字。

## Frontmatter

Frontmatter 是 Markdown 文件開頭一段用 --- 包裹起來的內容，其內部使用 yml 語法。通過 Frontmatter，我們可以標識文件的編輯時間，使用的圖標，分類，標籤等等。

::: details 示例

```markdown
---
date: 1919-08-10
icon: jam:write-f
order: 1
---

# 文件標題

...
```

:::

各字段含義如下：

- `date` 文件的編輯時間
- `icon` 文件標題旁邊的圖標
- `order` 文件在側邊欄中的排序
