---
order: 6
icon: jam:write-f
---

# 文件編寫指南

::: tip
本文件的目的在於指導文件編寫者更好地使用主題提供的功能，以此達到更易讀的效果。
:::

我們的文件基於 [VuePress](https://github.com/vuejs/vuepress) 建置，使用了 [vuepress-theme-plume](https://github.com/pengzhanbo/vuepress-theme-plume) 主題。您也可以查看[官方文件](https://theme-plume.vuejs.press/)來獲取更詳細的說明。這裡僅介紹一些常用的功能。

## 在地開發

1. 安裝 [pnpm](https://pnpm.io/installation)，並參考 [Pull Request 流程簡述](./development.md#github-pull-request-流程簡述)將儲存庫複製（Clone）到在地。
2. 在 `docs` 目錄下開啟終端機，執行 `pnpm i` 安裝依賴項目。
3. 執行 `pnpm run dev` 啟動開發伺服器。

## 容器與卡片

該主題提供了關於提示、註釋、資訊、注意、警告和詳情自定義容器的支援，我們可以利用這一特性來強調部分內容。

容器的使用方法：

```markdown
::: [容器類型] [容器標題（選填）]
您想寫的內容
:::
```

或是使用 GitHub 風格語法：

```markdown
> [!容器類型]
> 您想寫的內容
```

可接受的容器類型與其預設標題如下：

- `tip` 提示
- `note` 註釋
- `info` 相關資訊
- `warning` 注意
- `danger` 警告
- `details` 詳情
- `demo-warpper` ==特殊容器==

### 容器範例

::: tip
這是提示容器
:::

::: note
這是註釋容器
:::

::: info
這是資訊容器
:::

::: warning
這是注意容器
:::

::: danger
這是警告容器
:::

::: details
這是詳情容器
:::

::: demo-wrapper
這是一個很特殊的容器
:::

## 麥克筆標記

您可以使用標記語法來對想要顯示的內容進行標記，用於強調重點事項。

使用方法：用 `==標記內容=={標記顏色（選填）}` 的語法進行標記，請注意標記兩邊需要有空格。

**輸入：**

```markdown
MaaAssistantArknights 是由 ==很多豬== 開發的
```

**輸出：**

MaaAssistantArknights 是由 ==很多豬== 開發的

主題還內建了以下的配色方案：

- **default**: `==Default==` - ==Default==
- **info**: `==Info=={.info}` - ==Info=={.info}
- **note**: `==Note=={.note}` - ==Note=={.note}
- **tip**: `==Tip=={.tip}` - ==Tip=={.tip}
- **warning**: `==Warning=={.warning}` - ==Warning=={.warning}
- **danger**: `==Danger=={.danger}` - ==Danger=={.danger}
- **caution**: `==Caution=={.caution}` - ==Caution=={.caution}
- **important**: `==Important=={.important}` - ==Important=={.important}

## 隱藏文字

出於某種原因，您可能需要將文件的某部分暫時塗黑，在這種情況下您可以使用隱藏文字功能。

您可以使用 `!!需要隱秘的內容!!{配置（選填）}` 的語法來使用，預設效果如下：

!!總感覺在看萌娘百科（劃掉!!

有以下配置可以使用：

::: demo-wrapper
輸入：

```markdown
+ 遮罩層效果 + 滑鼠懸停：!!滑鼠懸停看到我了!!{.mask .hover}
+ 遮罩層效果 + 點擊：!!點擊看到我了!!{.mask .click}
+ 文字模糊效果 + 滑鼠懸停：!!滑鼠懸停看到我了!!{.blur .hover}
+ 文字模糊效果 + 點擊：!!點擊看到我了!!{.blur .click}
```

輸出：

- 遮罩層效果 + 滑鼠懸停：!!滑鼠懸停看到我了!!{.mask .hover}
- 遮罩層效果 + 點擊：!!點擊看到我了!!{.mask .click}
- 文字模糊效果 + 滑鼠懸停：!!滑鼠懸停看到我了!!{.blur .hover}
- 文字模糊效果 + 點擊：!!點擊看到我了!!{.blur .click}

:::

## 步驟

當您正在撰寫一個步驟化的教學時，有序列表可能會因為嵌套而失去層次感，這種時候 `steps` 容器就是最好的選擇。

注意該容器用四個冒號來標記開始和結束，與常規的容器不同。

輸入：

````markdown
:::: steps
1. 步驟 1

   ```ts
   console.log('Hello World!')
   ```

2. 步驟 2

   這裡是步驟 2 的相關內容

3. 步驟 3

   ::: tip
   提示容器
   :::

4. 結束
::::
````

輸出：

:::: steps
1. 步驟 1

   ```ts
   console.log('Hello World!')
   ```

2. 步驟 2

   這裡是步驟 2 的相關內容

3. 步驟 3

   ::: tip
   提示容器
   :::

4. 結束
::::

## 智慧圖片容器

我們基於主題提供的功能包裝了一個圖片容器。該容器能夠在亮暗主題下自動顯示對應主題的圖片，同時支援自動佈局。

您可以在 Markdown 正文中使用 `<ImageGrid>` 組件來呼叫該方法，具體的語法和效果如下：

::: demo-wrapper

這是語法：

```markdown
<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/readme/1-light.png',
    dark: 'images/zh-cn/readme/1-dark.png'
  },
  {
    light: 'images/zh-cn/readme/2-light.png',
    dark: 'images/zh-cn/readme/2-dark.png'
  },
  {
    light: 'images/zh-cn/readme/3-light.png',
    dark: 'images/zh-cn/readme/3-dark.png'
  },
  {
    light: 'images/zh-cn/readme/4-light.png',
    dark: 'images/zh-cn/readme/4-dark.png'
  }
]" />
```

這是渲染效果：

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/readme/1-light.png',
    dark: 'images/zh-cn/readme/1-dark.png'
  },
  {
    light: 'images/zh-cn/readme/2-light.png',
    dark: 'images/zh-cn/readme/2-dark.png'
  }
]" />

:::

## 欄位容器

該語法較為複雜，請參考 [官方文件](https://theme-plume.vuejs.press/guide/markdown/field/) 進行使用。

效果展示如下：

:::: field-group
::: field name="theme" type="ThemeConfig" required default="{ base: '/' }"
主題配置
:::

::: field name="enabled" type="boolean" optional default="true"
是否啟用
:::

::: field name="callback" type="(...args: any[]) => void" optional default="() => {}"
<Badge type="tip" text="v1.0.0 新增"  />
回呼函數
:::

::: field name="other" type="string" deprecated
<Badge type="danger" text="v0.9.0 棄用"  />
已棄用屬性
:::
::::

## 圖示

該主題提供了圖示支援，您可以在以下地方使用圖示：

- 文件標題：在 frontmatter 中設定文件標題旁邊的圖示。

- 導覽列 / 側邊欄：設定在導覽列與側邊欄中顯示的圖示。

- 文件內容：在文件內部使用圖示。

### 設定文件的圖示

您可以在文件的 [frontmatter](#frontmatter) 中使用 `icon` 來設定文件的圖示。

這個圖示會顯示在文件標題的旁邊。

::: details 本文件的 frontmatter 設定

```markdown
---
icon: jam:write-f
---
```

:::

### 在文件中使用圖示

您可以使用 `<Icon />` 組件在 Markdown 中添加圖示。該組件有以下屬性：

- `icon` 接受圖示關鍵字及 URL，如 `jam:write-f`，`ic:round-home` 等。
- `color` 接受 CSS 風格的顏色值，如 `#fff`，`red` 等（該選項僅對 SVG 圖示有效）。
- `size` 接受 CSS 風格的大小，如 `1rem`，`2em`，`100px` 等。

::: demo-wrapper 案例

輸入：

```markdown
- home - <Icon name="material-symbols:home" color="currentColor" size="1em" />
- vscode - <Icon name="skill-icons:vscode-dark" size="2em" />
- twitter - <Icon name="skill-icons:twitter" size="2em" />
```

輸出：

- home - <Icon name="material-symbols:home" color="currentColor" size="1em" />
- vscode - <Icon name="skill-icons:vscode-dark" size="2em" />
- twitter - <Icon name="skill-icons:twitter" size="2em" />

:::

### 圖示關鍵字的獲取

本文件使用的圖示來自於 [iconify](https://iconify.design/)，您可以在其提供的 [圖示搜尋介面](https://icon-sets.iconify.design/) 中搜尋您想要的圖示，然後複製其關鍵字。

## Frontmatter

Frontmatter 是 Markdown 文件開頭一段用 `---` 包裹起來的內容，其內部使用 YAML 語法。透過 Frontmatter，我們可以標示文件的編輯時間、使用的圖示、分類、標籤等等。

::: details 範例

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

各欄位含義如下：

- `date`：文件的編輯時間
- `icon`：文件標題旁邊的圖示
- `order`：文件在側邊欄中的排序
