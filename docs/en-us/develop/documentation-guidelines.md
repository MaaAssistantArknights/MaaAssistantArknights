---
order: 6
icon: jam:write-f
---

# Document Writing Guide

::: tip
The purpose of this document is to guide document writers to better utilize the functionalities provided by the theme, thus achieving a more readable effect.
:::

Our documentation is built on [vuepress](https://github.com/vuejs/vuepress) and utilizes the [vuepress-theme-hope](https://github.com/vuepress-theme-hope/vuepress-theme-hope) theme. You can also refer to the [official documentation](https://theme-hope.vuejs.press/en/) for more detailed explanations. Here, we only introduce some common functionalities.

## Deploy Locally

1. Install [Pnpm](https://pnpm.io/installation), and clone the repository refer to [Introduction to GitHub Pull request Flow](./development.md#introduction-to-github-pull-request-flow).
2. Create a terminal in the `website` directory, then run `pnpm i` to download dependencies.
3. Run `pnpm run dev` to deploy.

## Containers

~~Not the docker container~~

This theme provides support for custom containers such as tips, notes, info, warnings, alerts, and details. We can utilize this feature to emphasize certain content.

Usage of containers:

```markdown
::: [Container Type] [Container Title (optional)]
Content you want to write
:::
```

Accepted container types and their default titles are as follows:

- `tip` Tip
- `note` note
- `info` info
- `warning` warning
- `danger` danger
- `details` details

### Container Examples

::: tip
This is a tip container
:::

::: note
This is a note container
:::

::: info
This is an info container
:::

::: warning
This is a warning container
:::

::: danger
This is a danger container
:::

::: details
This is a details container
:::

## Icons

This theme provides icon support, where you can use icons in the following places:

- Document title: Set the icon next to the document title in frontmatter.

- Navbar/Sidebar: Set icons displayed in the navbar and sidebar.

- Document content: Use icons in the document content.

### Setting Document Icons

You can use `icon` in the document's [frontmatter](#frontmatter) to set the document's icon.

This icon will be displayed next to the document title.

::: details Frontmatter settings of this document

```markdown
---
icon: jam:write-f
---
```

:::

### Using Icons in Documents

You can use the `<HopeIcon />` component to add icons in markdown. This component has the following attributes:

- `icon`: Accepts icon keywords and URLs, such as `jam:write-f`, `ic:round-home`, etc.
- `color`: Accepts CSS-style color values, such as `#fff`, `red`, etc. (This option only works for SVG icons).
- `size`: Accepts CSS-style sizes, such as `1rem`, `2em`, `100px`, etc.

::: details Example
<HopeIcon icon="ic:round-home" color="#1f1e33"/>

```markdown
<HopeIcon icon="ic:round-home" color="#1f1e33"/>
```

<HopeIcon icon="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" size="4rem" />
```markdown
<HopeIcon icon="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" size="4rem" />
```
:::

### Obtaining Icon Keywords

The icons used in this document are from [iconify](https://iconify.design/). You can search for the icons you want in its provided [icon search interface](https://icon-sets.iconify.design/) and then copy their keywords.

## Frontmatter

Frontmatter is the content at the beginning of a Markdown document enclosed in `---,` using YAML syntax internally. Through frontmatter, we can identify the document's editing time, the icon used, the classification, tags, etc.

::: details Example

```markdown
---
date: 1919-08-10
icon: jam:write-f
order: 1
---

# Document Title

...
```

:::

The meanings of each field are as follows:

- `date`: Editing time of the document
- `icon`: Icon next to the document title
- `order`: Document order in the sidebar
